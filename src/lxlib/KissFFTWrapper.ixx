module;

#include <vector>

export module lxlib.KissFFTWrapper;
#include <kissfft/kiss_fftnd.h>
#include <complex>

import lxlib.Array2D;

export template<class TComponentType>
class KissFFTWrapper {
public:
	using Complex = std::complex<TComponentType>;
	// complex-to-complex FFT
    static Array2D<Complex> fftC2C(Array2D<Complex> const& in) {
		return fftImpl(in, false);
    }
    static Array2D<Complex> normalize(Array2D<Complex> const& in) {
        Array2D<Complex> out(in.Size(), nofill());
        for (int i = 0; i < in.area; i++) {
            out.data()[i] = in.data()[i] / static_cast<TComponentType>(in.area);
        }
        return out;
	}
    // complex-to-complex inverse FFT
    static Array2D<Complex> inverseFftC2C(Array2D<Complex> const& in) {
        return fftImpl(in, true);
    }

private:
    static Array2D<Complex> fftImpl(Array2D<Complex> const& in, bool backward) {
        const std::vector<int> dims = { in.h, in.w };
        kiss_fftnd_cfg cfg = kiss_fftnd_alloc(dims.data(), 2, backward ? 1 : 0, nullptr, nullptr);
        Array2D<Complex> out(in.Size(), nofill());

        //kissfftnd<Complex> fft(dims, backward);

        //fft.transform(in_img.data(), out_img.data());

        kiss_fftnd(cfg,
            reinterpret_cast<const kiss_fft_cpx*>(in.data()),
            reinterpret_cast<kiss_fft_cpx*>(out.data()));

        kiss_fft_free(cfg);
        return out;
    }
};
