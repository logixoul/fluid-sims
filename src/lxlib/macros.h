#pragma once

#define GPU_SCOPE(name) GpuScope gpuScope_ ## __LINE__  (name);
#define MULTILINE(...) #__VA_ARGS__
#define forxy(image) \
	for(ivec2 p(0, 0); p.y < image.h; p.y++) \
		for(p.x = 0; p.x < image.w; p.x++)