#pragma once

#pragma region Application

// This is a global handle for the application used to show errors in a message box.
static HWND hWnd;

// Represents different error code this engine can report.
enum CA4G_Errors {
	// Some error occurred.
	CA4G_Errors_Any,
	// The Pipeline State Object has a bad construction.
	CA4G_Errors_BadPSOConstruction,
	// The Signature has a bad construction
	CA4G_Errors_BasSignatureConstruction,
	// The image is in an unsupported format.
	CA4G_Errors_UnsupportedFormat,
	// Some shader were not found
	CA4G_Errors_ShaderNotFound,
	// Some initialization failed because memory was over
	CA4G_Errors_RunOutOfMemory,
	// Invalid Operation
	CA4G_Errors_Invalid_Operation,
	// Fallback raytracing device was not supported
	CA4G_Errors_Unsupported_Fallback
};

// Shows a message box with a user-readable message for each error.
CA4G_Errors AfterShow(CA4G_Errors error, const wchar_t * arg = nullptr, HRESULT hr = S_OK) {

	static wchar_t fullMessage[1000];
	
	ZeroMemory(&fullMessage, sizeof(fullMessage));

	static wchar_t * errorMessage;

	switch (error) {
	case CA4G_Errors_BadPSOConstruction:
		errorMessage = TEXT("Error in PSO construction. Check all registers are bound and the input layout is compatible.");
		break;
	case CA4G_Errors_UnsupportedFormat:
		errorMessage = TEXT("Error in Format. Check resource format is supported.");
		break;
	case CA4G_Errors_ShaderNotFound:
		errorMessage = TEXT("Shader file was not found. Check file path in pipeline binding.");
		break;
	case CA4G_Errors_RunOutOfMemory:
		errorMessage = TEXT("Run out of memory during object creation.");
		break;
	case CA4G_Errors_Invalid_Operation:
		errorMessage = TEXT("Invalid operation.");
		break;
	case CA4G_Errors_Unsupported_Fallback:
		errorMessage = TEXT("Fallback raytracing device was not supported. Check OS is in Developer Mode.");
		break;
	default:
		errorMessage = TEXT("Unknown error in CA4G");
		break;
	}
	
	lstrcatW(fullMessage, errorMessage);
	lstrcatW(fullMessage, arg);

	if (FAILED(hr)) // Some HRESULT to show
	{
		_com_error err(hr);
		lstrcatW(fullMessage, err.ErrorMessage());
	}

	MessageBox(hWnd, fullMessage, TEXT("Error"), 0);

	return error;
}

#pragma endregion