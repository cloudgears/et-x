//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#pragma once

#include <et/timers/notifytimer.h>
#include <et/camera/camera.h>

namespace demo
{
	class CameraController
	{
	public:
		CameraController();
		
		void init(et::RenderContext*);
		
		const et::Camera& camera() const
			{ return _mainCamera; }

		et::Camera& camera()
			{ return _mainCamera; }

		void adjustCameraToNextContextSize(const et::vec2&);
		
		void handlePressedKey(size_t);
		void handleReleasedKey(size_t);
		
		void handlePointerDrag(const et::vec2&);
		
	private:
		et::Camera _mainCamera;
		et::NotifyTimer _updateTimer;
		et::vec2 _movements;
		bool _boostEnabled = false;
	};
}
