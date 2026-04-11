module;
#include "precompiled.h"

export module lxlib.Rect;

export namespace lx {
	template<class T>
	class Rect {
	public:
       using Self = lx::Rect<T>;
		T x1;
		T y1;
		T x2;
		T y2;
        static Self fromBounds(glm::tvec2<T> const& topLeft, glm::tvec2<T> const& bottomRight) {
			Self r;
			r.x1 = topLeft.x;
			r.y1 = topLeft.y;
			r.x2 = bottomRight.x;
			r.y2 = bottomRight.y;
			return r;
		}
		T width() const {
			return x2 - x1;
		}
		T height() const {
			return y2 - y1;
		}
		glm::tvec2<T> topLeft() const {
			return glm::tvec2<T>(x1, y1);
		}
		glm::tvec2<T> bottomRight() const {
			return glm::tvec2<T>(x2, y2);
		}
		glm::tvec2<T> size() const {
			return glm::tvec2<T>(width(), height());
		}
		void expand(T xe, T ye) {
			x1 -= xe;
			y1 -= ye;
			x2 += xe;
			y2 += ye;
		}
	};
}
