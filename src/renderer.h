#pragma once

#include <thread>

#include "raylib.h"
#include "raymath.h"
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
		Renderer(World& w, const RendererConfig& cfg, const Vector2& winSize, const std::string& winName);
		~Renderer();

		bool done = false;

	private:
		std::thread _redrawer;

		World& _w;
		
		RendererConfig _cfg;

		Vector2 _windowSize, _baseWindowSize;
		std::string _windowName;
		float _lastResizeTime;

		RenderPyramid _pyramid;

		Shader _shader;
		Texture2D _shTex;
		float _time;

		Vector3 _light;

		Camera _cam;

		void _resetPlayerPos();

		void _init();
		void _updateShaderSize();
		void _render();
	};
}