#pragma once

#include <thread>
#include <string>

#include "raylib.h"
#include "world.h"

#include <rcamera.h>

namespace trivox
{
	struct RendererConfig {
        uint32_t radius;
        std::pair<int8_t, int8_t> minMaxPow;
	};

	class Renderer
	{
	public:
		Renderer(World::Ptr w, const RendererConfig& cfg, const Vector2& winSize, const std::string& winName);
		~Renderer();

		bool done = false;

	private:
		std::thread _redrawer;

		World::Ptr _w;
		
		RendererConfig _cfg;

		Vector2 _windowSize, _baseWindowSize;
		std::string _windowName;
		float _lastResizeTime;

		Shader _shader;
		Texture2D _shTex;
		float _time;

		Vector3 _light;

		Camera _cam;

		void _resetPlayerPos();

		void _init();
		void _updateShaderSize();
		void _moveCamera();
		void _input();
		void _render();
	};
}