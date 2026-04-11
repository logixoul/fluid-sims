#pragma once

#define GPU_SCOPE(name) lx::GpuScope gpuScope_ ## __LINE__  (name);
#define MULTILINE(...) #__VA_ARGS__
