#pragma once
#include <dxut\cmmn.h>
#include <Xinput.h>
#undef max
#undef min

namespace dxut {
	inline float clamp(float x, float max = 1.0f, float min = -1.0f)
	{
		if (x > max) return max;
		else if (x < min) return min;
		return x;
	}

	struct gamepad {
		uint32_t index; 
		XINPUT_STATE raw_state;
		bool connected;

		gamepad(uint32_t index_) : index(index_), connected(false) {
		}
		
		void update() {
			connected = XInputGetState(index, &raw_state) == ERROR_SUCCESS;
		}

		inline XMFLOAT2 left_thumb() {
			return XMFLOAT2(
				dpc<short>(raw_state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE),
				dpc<short>(raw_state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
		}

		inline XMFLOAT2 right_thumb() {
			return XMFLOAT2(
				dpc<short>(raw_state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE),
				dpc<short>(raw_state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
		}

		inline float left_trigger() {
			return dpc<BYTE>(raw_state.Gamepad.bLeftTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
		}

		inline float right_trigger() {
			return dpc<BYTE>(raw_state.Gamepad.bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
		}

		inline bool is_button_down(unsigned short button) const
		{
			return (raw_state.Gamepad.wButtons & button) != 0;
		}
	private:
		template <typename T>
		inline float dpc(T v, T z, float max = numeric_limits<T>::max()) const
		{
			float vn = clamp(static_cast<float>(v) / max);
			if (abs(vn) < (float)z / max)
				vn = 0;
			return vn;
		}
	};
}