module;
#include "../precompiled.h"
export module MultiscaleGrowthSketch.tests;
import lxlib.Array2D;
import ThisSketch_ImageProcessingHelpers;
import gpuBlurClaude;
import lxlib.stuff;

export namespace MultiscaleGrowthSketch::tests {
	void testMatchingFunctionality() {
		lx::Array2D<float> arr(100, 100);
		for (auto p : arr.coords()) {
			arr(p) = lx::randFloat();
		}

		std::vector<int> testSizes{ 50, 200, 67, 107, 3 };
		for (int testSize : testSizes) {
			//auto newImpl = ThisSketch::resize_referenceImplementation(arr, ivec2(testSize, testSize)); // works
			auto newImpl = gpuBlurClaude::singleblurLikeCinder(arr, ivec2(testSize, testSize));
			auto oldImpl = ThisSketch::resize_referenceImplementation(arr, ivec2(testSize, testSize));
			//mm("new", newImpl);
			//mm("old", oldImpl);

			for (auto p : newImpl.coords()) {
				if (std::abs(newImpl(p) - oldImpl(p)) > 0.0001) {
					std::cout << "[" << testSize << "] mismatch at " << p.x << ", " << p.y << ": " << newImpl(p) << " vs " << oldImpl(p) << std::endl;
				}
			}
		}
	}
}