//
// Created by Sergo on 10.01.2021.
//

#include "zstandard/zstandard.h"
#include "zstandard/zTypes.h"

zMatrix zMatrix::_identity(1, 0, 0, 0,  0, 1, 0, 0,
                           0, 0, 1, 0,  0, 0, 0, 1);
zColor zColor::red(0xffff0000);
zColor zColor::green(0xff00ff00);
zColor zColor::blue(0xff0000ff);
zColor zColor::white(0xffffffff);
zColor zColor::gray(0xff808080);
zColor zColor::shadow(0xaf101010);
zColor zColor::black(0xff000000);

static u32 z_hex(u8 ch) {
	if(ch >= '0' && ch <= '9') ch -= 48;
	else if(ch >= 'A' && ch <= 'F') ch -= 55;
	else if(ch >= 'a' && ch <= 'f') ch -= 87;
	else ch = 0;
	return ch;
}

zColor::zColor(cstr str) {
	u32 argb(0xffffffff);
	auto l(z_strlen(str));
	if(l > 7) l = 8;
	int tetra(15);
	for(int i = 0; i < l; i++) {
		auto ch(z_hex(str[(l - i) - 1]));
		argb &= ~tetra; argb |= (ch << (i << 2));
		tetra <<= 4;
	}
	set(argb);
}

void zColor::set(u32 rgba) {
	vec[0] = (float)((rgba & 0x000000ffU) >>  0U) / 255.0f;
	vec[1] = (float)((rgba & 0x0000ff00U) >>  8U) / 255.0f;
	vec[2] = (float)((rgba & 0x00ff0000U) >> 16U) / 255.0f;
	vec[3] = (float)((rgba & 0xff000000U) >> 24U) / 255.0f;
}

u32 zColor::toABGR() const {
	auto _r((int)(r * 255.0f));
	auto _g((int)(g * 255.0f));
	auto _b((int)(b * 255.0f));
	auto _a((int)(a * 255.0f));
	return _r | (_g << 8) | (_b << 16) | (_a << 24);
}

u32 zColor::toARGB() const {
	auto _r((int)(r * 255.0f));
	auto _g((int)(g * 255.0f));
	auto _b((int)(b * 255.0f));
	auto _a((int)(a * 255.0f));
	return _b | (_g << 8) | (_r << 16) | (_a << 24);
}

const zColor& zColor::from(cstr s) {
	r = z_ston<float>(s, RADIX_FLT); s = z_strAfter(s, ',');
	if(s) g = z_ston<float>(s, RADIX_FLT), s = z_strAfter(s, ','); else g = r;
	if(s) b = z_ston<float>(s, RADIX_FLT), s = z_strAfter(s, ','); else b = g;
	if(s) a = z_ston<float>(s, RADIX_FLT); else a = b;
	return *this;
}

u8* zColor::state(u8 *ptr, bool save) {
	if(save) {
		auto _r(((u32)(r * 255.0f) & 0xff) <<  0);
		auto _g(((u32)(g * 255.0f) & 0xff) <<  8);
		auto _b(((u32)(b * 255.0f) & 0xff) << 16);
		auto _a(((u32)(a * 255.0f) & 0xff) << 24);
		dwordLE(&ptr, _r | _g | _b | _a);
	} else {
		set(dwordLE(&ptr));
	}
	return ptr;
}

template<> u8* zPoint<int>::state(u8* ptr, bool save) {
	if(save) {
		dwordLE(&ptr, x); dwordLE(&ptr, y);
	} else {
		x = (int)dwordLE(&ptr); y = (int)dwordLE(&ptr);
	}
	return ptr;
}

template<> u8* zSize<int>::state(u8* ptr, bool save) {
	if(save) {
		dwordLE(&ptr, w); dwordLE(&ptr, h);
	} else {
		w = (int)dwordLE(&ptr); h = (int)dwordLE(&ptr);
	}
	return ptr;
}

template<> u8* zRect<int>::state(u8 *ptr, bool save) {
	if(save) {
		dwordLE(&ptr, x); dwordLE(&ptr, y);
		dwordLE(&ptr, w); dwordLE(&ptr, h);
	} else {
		x = (int)dwordLE(&ptr); y = (int)dwordLE(&ptr);
		w = (int)dwordLE(&ptr); h = (int)dwordLE(&ptr);
	}
	return ptr;
}

template<> zRect<int> zRect<int>::dimens(float sw) {
	auto _x = (int)round((float)x * sw), _y = (int)round((float)y * sw);
	auto _w = (int)round((float)w * sw), _h = (int)round((float)h * sw);
	return zRect<int>(_x, _y, _w, _h);
}

template<> const zRect<int>& zRect<int>::from(cstr s) {
	x = z_ston(s, RADIX_DEC); s = z_strAfter(s, ',');
	if(s) y = z_ston(s, RADIX_DEC), s = z_strAfter(s, ','); else y = x;
	if(s) w = z_ston(s, RADIX_DEC), s = z_strAfter(s, ','); else w = y;
	if(s) h = z_ston(s, RADIX_DEC); else h = w;
	return *this;
}

template<> const zRect<float>& zRect<float>::from(cstr s) {
	x = z_ston<float>(s, RADIX_FLT); s = z_strAfter(s, ',');
	if(s) y = z_ston<float>(s, RADIX_FLT), s = z_strAfter(s, ','); else y = x;
	if(s) w = z_ston<float>(s, RADIX_FLT), s = z_strAfter(s, ','); else w = y;
	if(s) h = z_ston<float>(s, RADIX_FLT); else h = w;
	return *this;
}

zVec3 operator * (const zQuat& q, const zVec3& v) {
	zVec3 qvec(q.x, q.y, q.z), uv(qvec.cross(v)), uuv(qvec.cross(uv));
	uv *= (2.0f * q.w); uuv *= 2.0f;
	return v + uv + uuv;
}

zMatrix::zMatrix(float _f11, float _f12, float _f13, float _f14, float _f21, float _f22, float _f23, float _f24,
				 float _f31, float _f32, float _f33, float _f34, float _f41, float _f42, float _f43, float _f44) {
	_11 = _f11; _12 = _f12; _13 = _f13; _14 = _f14;
	_21 = _f21; _22 = _f22; _23 = _f23; _24 = _f24;
	_31 = _f31; _32 = _f32; _33 = _f33; _34 = _f34;
	_41 = _f41; _42 = _f42; _43 = _f43; _44 = _f44;
}

float zMatrix::MINOR(const zMatrix& m, int r0, int r1, int r2, int c0, int c1, int c2) const {
	float* f(m);
	return  f[r0 * 4 + c0] * (f[r1 * 4 + c1] * f[r2 * 4 + c2] - f[r2 * 4 + c1] * f[r1 * 4 + c2]) -
			f[r0 * 4 + c1] * (f[r1 * 4 + c0] * f[r2 * 4 + c2] - f[r2 * 4 + c0] * f[r1 * 4 + c2]) +
			f[r0 * 4 + c2] * (f[r1 * 4 + c0] * f[r2 * 4 + c1] - f[r2 * 4 + c0] * f[r1 * 4 + c1]);
}

float zMatrix::determinant() const {
	return	_11 * MINOR( * this, 1, 2, 3, 1, 2, 3) -
			_12 * MINOR( * this, 1, 2, 3, 0, 2, 3) +
			_13 * MINOR( * this, 1, 2, 3, 0, 1, 3) -
			_14 * MINOR( * this, 1, 2, 3, 0, 1, 2);
}

zMatrix zMatrix::adjoint() const {
	return zMatrix(	MINOR( *this, 1, 2, 3, 1, 2, 3), -MINOR(*this, 0, 2, 3, 1, 2, 3),
			   		MINOR( *this, 0, 1, 3, 1, 2, 3), -MINOR(*this, 0, 1, 2, 1, 2, 3),
			   		-MINOR(*this, 1, 2, 3, 0, 2, 3), MINOR( *this, 0, 2, 3, 0, 2, 3),
			   		-MINOR(*this, 0, 1, 3, 0, 2, 3), MINOR( *this, 0, 1, 2, 0, 2, 3),
			   		MINOR( *this, 1, 2, 3, 0, 1, 3), -MINOR(*this, 0, 2, 3, 0, 1, 3),
			   		MINOR( *this, 0, 1, 3, 0, 1, 3), -MINOR(*this, 0, 1, 2, 0, 1, 3),
			   		-MINOR(*this, 1, 2, 3, 0, 1, 2), MINOR( *this, 0, 2, 3, 0, 1, 2),
			   		-MINOR(*this, 0, 1, 3, 0, 1, 2), MINOR( *this, 0, 1, 2, 0, 1, 2));
}

const zMatrix& zMatrix::view(const zVec3& pos, const zVec3& at, const zVec3& up) {
	zVec3 vLook(pos - at); vLook.normalize();
	zVec3 vRight(up.cross(vLook)); vRight.normalize();
	zVec3 vUp(vLook.cross(vRight)); vUp.normalize();
	_11 = vRight.x ; _12 = vUp.x ; _13 = vLook.x ; _14 = 0.0f;
	_21 = vRight.y ; _22 = vUp.y ; _23 = vLook.y ; _24 = 0.0f;
	_31 = vRight.z ; _32 = vUp.z ; _33 = vLook.z ; _34 = 0.0f;
	_41 = -vRight.dot(pos); _42 = -vUp.dot(pos); _43 = -vLook.dot(pos); _44 = 1.0f;
	return *this;
	/*
  Vec3 vec_forward, vec_up_norm, vec_side;
  Mat4 result;

  vec_forward.x_ = vec_eye.x_ - vec_at.x_;
  vec_forward.y_ = vec_eye.y_ - vec_at.y_;
  vec_forward.z_ = vec_eye.z_ - vec_at.z_;

  vec_forward.Normalize();
  vec_up_norm = vec_up;
  vec_up_norm.Normalize();
  vec_side = vec_up_norm.Cross(vec_forward);
  vec_up_norm = vec_forward.Cross(vec_side);

  result.f_[0] = vec_side.x_;
  result.f_[4] = vec_side.y_;
  result.f_[8] = vec_side.z_;
  result.f_[12] = 0;
  result.f_[1] = vec_up_norm.x_;
  result.f_[5] = vec_up_norm.y_;
  result.f_[9] = vec_up_norm.z_;
  result.f_[13] = 0;
  result.f_[2] = vec_forward.x_;
  result.f_[6] = vec_forward.y_;
  result.f_[10] = vec_forward.z_;
  result.f_[14] = 0;
  result.f_[3] = 0;
  result.f_[7] = 0;
  result.f_[11] = 0;
  result.f_[15] = 1.0;

  result.PostTranslate(-vec_eye.x_, -vec_eye.y_, -vec_eye.z_);
  return result; 	 *
	 */
}

const zMatrix& zMatrix::fromQuat(const zQuat& q) {
	float x2(q.x * q.x * 2.0f), y2(q.y * q.y * 2.0f), z2(q.z * q.z * 2.0f);
	float xy(q.x * q.y * 2.0f), yz(q.y * q.z * 2.0f), zx(q.z * q.x * 2.0f);
	float xw(q.x * q.w * 2.0f), yw(q.y * q.w * 2.0f), zw(q.z * q.w * 2.0f);
	_11 = 1.0f - y2 - z2; _12 = xy + zw; _13 = 0; _14 = zx - yw;
	_21 = xy - zw; _22 = 1.0f - z2 - x2; _23 = yz + xw; _24 = 0;
	_31 = zx + yw; _32 = yz - xw; _33 = 1.0f - x2 - y2; _34 = 0;
	_41 = 0; _42 = 0; _43 = 0; _44 = 1.0f;
	return *this;
}

const zMatrix& zMatrix::world(const zVec3& p, const zVec3& scale, const zQuat& orientation) {
	zMatrix pos, sc, rot(orientation);
	pos.translate(p); sc.scale(scale);
	*this = sc * rot * pos;
	return *this;
}

const zMatrix& zMatrix::ortho2D(int left, int top, int right, int bottom) {
	float near(-1.0f), far(1.0f);
	float x(1.0f / (float)(right - left)), y(1.0f / (float)(bottom - top)), z(1.0f / (float)(far - near));
	identity();
	_11 = 2.0f * x; _22 = 2.0f * y; _33 = -2.0f * z;
	_41 = -(right + left) * x; _42 = (top + bottom) * y; _43 = -(far + near) * z;
	return *this;
}

const zMatrix& zMatrix::ortho(int left, int right, int top, int bottom) {
    identity(); float tb((float)(top - bottom)), rl((float)(right - left));
    _11 = 2.0f / rl;
    _22 = 2.0f / tb;
    _33 = 1.0f;
    _41 = -(float)(right + left) / rl;
    _42 = -(float)(top + bottom) / tb;
	return *this;
}

const zMatrix& zMatrix::perspective(float w, float h, float zn, float zf) {
	memset(*this, 0, sizeof(zMatrix));
	float n2(zn * 2.0f), _n2(1.0f / (zn - zf));
	_11 = n2 / w; _22 = n2 / h; _33 = (zf + zn) * _n2; _34 = -1.0f; _43 = zf * _n2 * n2;
	return *this;
}

const zMatrix& zMatrix::perspectiveFov(float fovy, float aspect, float zn, float zf) {
	float h(cosf(fovy / 2.0f) / sinf(fovy / 2.0f)), w(h / aspect), f(zf / (zf - zn));
	memset(*this, 0, sizeof(zMatrix));
	_11 = w; _22 = h; _33 = f;
	_34 = 1.0f; _43 = -zn * f;
	return *this;
}

auto zQuat::operator *= (const zQuat& q) {
	float xx(w * q.x + x * q.w + y * q.z - z * q.y);
	float yy(w * q.y + y * q.w + z * q.x - x * q.z);
	float zz(w * q.z + z * q.w + x * q.y - y * q.x);
	float ww(w * q.w - x * q.x - y * q.y - z * q.z);
	x = xx; y = yy; z = zz; w = ww;
	return *this;
/*
	A = (q1->w + q1->x) * (q2->w + q2->x);
	B = (q1->z - q1->y) * (q2->y - q2->z);
	C = (q1->x - q1->w) * (q2->y + q2->z);
	D = (q1->y + q1->z) * (q2->x - q2->w);
	E = (q1->x + q1->z) * (q2->x + q2->y);
	F = (q1->x - q1->z) * (q2->x - q2->y);
	G = (q1->w + q1->y) * (q2->w - q2->z);
	H = (q1->w - q1->y) * (q2->w + q2->z);

	res->w = B + (-E - F + G + H) * 0.5;
	res->x = A - ( E + F + G + H) * 0.5;
	res->y =-C + ( E - F + G - H) * 0.5;
	res->z =-D + ( E - F - G + H) * 0.5;
*/
}

auto zQuat::operator * (const zQuat& q) const {
	return zQuat(w * q.x + x * q.w + y * q.z - z * q.y,
				w * q.y + y * q.w + z * q.x - x * q.z,
				w * q.z + z * q.w + x * q.y - y * q.x,
				w * q.w - x * q.x - y * q.y - z * q.z);
}

void zQuat::angleAxis(zVec3& axis, float& angle) const {
	float fSqrLength(x * x + y * y + z * z);
	if(fSqrLength > 0) {
		angle = 2 * acos(w);
		float fInvLength = sqrt(fSqrLength);
		axis.x = x / fInvLength; axis.y = y / fInvLength; axis.z = z / fInvLength;
	} else {
		angle = 0; axis.x = 1; axis.y = 0; axis.z = 0;
	}
}

zQuat zQuat::slerp(const zQuat& q, float t, bool shortestPath) const {
	auto fCos(dot(q)), fAngle(acos(fCos));
	if(fabs(fAngle) < Z_EPSILON) return *this;
	auto fSin(sinf(fAngle)), fInvSin(1.0f / fSin);
	auto fCoeff0(sin((1.0f - t) * fAngle) * fInvSin);
	auto fCoeff1(sin(t * fAngle) * fInvSin);
	if(fCos < 0 && shortestPath) {
		fCoeff0 = -fCoeff0;
		zQuat res(*this * fCoeff0 + q * fCoeff1);
		res.normalize();
		return res;
	}
	return *this * fCoeff0 + q * fCoeff1;
}

const zQuat& zQuat::rotateAxis(const zVec3& v, float theta) {
	auto fHalfAngle(theta / 2.0f), fSin(sinf(fHalfAngle));
	w = cosf(fHalfAngle); x = fSin * v.x; y = fSin * v.y; z = fSin * v.z;
	return *this;
}

const zQuat& zQuat::fromSpherical(float latitude, float longitude, float angle) {
	auto sin_a(sin(angle / 2.0f)), cos_a(cos(angle / 2.0f));
	auto sin_lat(sin(latitude)), cos_lat(cos(latitude));
	auto sin_long(sin(longitude)), cos_long(cos(longitude));
	x = sin_a * cos_lat * sin_long;
	y = sin_a * sin_lat;
	z = sin_a * sin_lat * cos_long;
	w = cos_a;
	return *this;
}

const zQuat& zQuat::fromMatrix(const zMatrix& mm) {
	float* f(mm);
	float tr(f[0] + f[5] + f[10]), s;
	if(tr > 0.0f) {
		s = sqrt (tr + 1.0f); w = s / 2.0f; s = 0.5f / s;
		x = (f[6] - f[9]) * s;
		y = (f[8] - f[2]) * s;
		z = (f[1] - f[4]) * s;
	} else {
		int nxt[3] = {1, 2, 0}, i(0); float q[4];
		if(f[5] > f[0]) i = 1;
		if(f[10] > f[i * 4 + i]) i = 2;
		int j(nxt[i]), k(nxt[j]);
		s = sqrt((f[i * 4 + i] - (f[j * 4 + j] + f[k * 4 + k])) + 1.0f);
		q[i] = s * 0.5f;
		if(s > Z_EPSILON) s = 0.5f / s;
		q[3] = (f[j * 4 + k] - f[k * 4 + j]) * s;
		q[j] = (f[i * 4 + j] + f[j * 4 + i]) * s;
		q[k] = (f[i * 4 + k] + f[k * 4 + i]) * s;
		x = q[0]; y = q[1]; z = q[2]; w = q[3];
	}
	return *this;
}

zVec3 zQuat::xAxis() const {
	auto fTy(2 * y), fTz(2 * z);
	return zVec3(1 - ((fTy * y) + (fTz * z)), (fTy * x) + (fTz * w), (fTz * x) - (fTy * w));
}

zVec3 zQuat::yAxis() const {
	auto fTx(2 * x), fTy(2 * y), fTz(2 * z);
	return zVec3((fTy * x) - (fTz * w), 1 - ((fTx * x) + (fTz * z)), (fTz * y) + (fTx * w));
}

zVec3 zQuat::zAxis() const {
	auto fTx(2.0f * x), fTy(2.0f * y), fTz(2.0f * z);
	return zVec3((fTz * x) + (fTy * w), (fTz * y) - (fTx * w), 1 - ((fTx * x) + (fTy * y)));
}

const zQuat& zQuat::inverse() {
	float f(x * x + y * y + z * z + w * w);
	if(f <= Z_EPSILON) {
		x = y = z = 0.0f; w = 1.0f;
	} else {
		f = 1.0f / f;
		x *= -f; y *= -f; z *= -f; w *= f;
	}
	return *this;
}

const zQuat& zQuat::exp() {
	float fAngle(sqrt(x * x + y * y + z * z));
	float fSin(sin(fAngle));
	if(fabs(fSin) >= Z_EPSILON) {
		float fCoeff(fSin / fAngle);
		x = fCoeff * x; y = fCoeff * y; z = fCoeff * z;
	}
	w = cos(fAngle);
	return *this;
}

const zQuat& zQuat::ln() {
	if(fabs(w) < 1.0f) {
		auto fAngle(acos(w)), fSin(sin(fAngle));
		if(fabs(fSin) >= Z_EPSILON) {
			float fCoeff(fAngle / fSin);
			x = fCoeff * x; y = fCoeff * y; z = fCoeff * z;
		}
	}
	w = 0.0f;
	return *this;
}

const zQuat& zQuat::angles(float yaw, float pitch, float roll) {
	float fSinYaw(sin(yaw / 2.0f)), fSinPitch(sin(pitch / 2.0f));
	float fSinRoll(sin(roll / 2.0f)), fCosYaw(cos(yaw / 2.0f));
	float fCosPitch(cos(pitch / 2.0f)), fCosRoll(cos(roll / 2.0f));
	x = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw;
	y = fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw;
	z = fSinRoll * fCosPitch * fCosYaw - fCosRoll * fSinPitch * fSinYaw;
	w = fCosRoll * fCosPitch * fCosYaw + fSinRoll * fSinPitch * fSinYaw;
	return *this;
}

zQuat zQuat::nlerp(const zQuat& q, float t, bool shortestPath) const {
	float fCos(dot(q));
	zQuat result((fCos < 0 && shortestPath) ? *this + t * ((-q) - *this) : *this + t * (q - *this));
	return result.normalize();
}

zQuat zQuat::squad(const zQuat& q1, const zQuat& q2, const zQuat& q3, float t, bool shortestPath) const {
	float slerpT(2.0f * t * (1.0f - t));
	zQuat slerpP(slerp(q3, t, shortestPath));
	zQuat slerpQ(q1.slerp(q2, t, false));
	return slerpP.slerp(slerpQ, slerpT, shortestPath);
}

auto zMatrix::inverse() {
	*this = adjoint();
	*this *= (1.0f / determinant());
	return *this;
}

const zMatrix& zMatrix::transpose() {
	zMatrix m(_11, _21, _31, _41,
			  _12, _22, _32, _42,
			  _13, _23, _33, _43,
			  _14, _24, _34, _44);
	*this = m; return *this;
}

const zMatrix& zMatrix::rotateEuler(float yaw, float pitch, float roll) {
	zQuat q; return fromQuat(q.angles(yaw, pitch, roll));
}

const zMatrix& zMatrix::rotateAngleAxis(const zVec3& v, float angle) {
	zQuat q; return fromQuat(q.rotateAxis(v, angle));
}

const zMatrix& zMatrix::postTranslate(float x, float y, float z) {
	_41 += (x * _11) + (y * _21) + (z * _31);
	_42 += (x * _12) + (y * _22) + (z * _32);
	_43 += (x * _13) + (y * _23) + (z * _33);
	_44 += (x * _14) + (y * _24) + (z * _34);
	return *this;
}

const zMatrix& zMatrix::rotate(float x, float y, float z) {
    static zMatrix my, mz; float fSin, fCos;
	identity();
    fSin = sinf(x); fCos = cosf(x);
    _22 = fCos; _23 = -fSin; _32 = fSin; _33 = fCos;
    fSin = sinf(y); fCos = cosf(y);
    my._11 = fCos; my._13 = fSin; my._31 = -fSin; my._33 = fCos;
    fCos = cosf(z); fSin = sinf(z);
    mz._11 = fCos; mz._12 = -fSin; mz._21 = fSin; mz._22 = fCos;
	*this *= my * mz;
	return *this;
}

const zMatrix& zMatrix::from3x3(float* f) {
	_v1.set(f[0], f[1], f[2], 0.0f); _v2.set(f[3], f[4], f[5], 0.0f);
	_v3.set(f[6], f[7], f[8], 0.0f); _v4.set(f[9], f[10], f[11], 1.0f);
	return *this;
}

float* z_vec3_mtx(float* v, float* m) {
	static float r[3];
	r[0] = v[0] * m[0] + v[1] * m[4] + v[2] * m[8]  + m[12];
	r[1] = v[0] * m[1] + v[1] * m[5] + v[2] * m[9]  + m[13];
	r[2] = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + m[14];
	return r;
}

float* z_vec4_mtx(float* v, float* m) {
	static float r[4];
	r[0] = v[0] * m[0] + v[1] * m[4] + v[2] * m[8]  + v[3] * m[12];
	r[1] = v[0] * m[1] + v[1] * m[5] + v[2] * m[9]  + v[3] * m[13];
	r[2] = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + v[3] * m[14];
	r[3] = v[0] * m[3] + v[1] * m[7] + v[2] * m[11] + v[3] * m[15];
	return r;
}

float* z_mtx_vec3(float* m, float* v3) {
	return z_vec3_mtx(v3, m);
}

float* z_mtx_vec4(float* m, float* v4) {
	return z_vec4_mtx(v4, m);
}

float* z_mtx_mtx(float* m1, float* m2) {
	static float m[16];

	m[0] = m1[0] * m2[0] + m1[1] * m2[4] + m1[2] * m2[8]  + m1[3] * m2[12];
	m[1] = m1[0] * m2[1] + m1[1] * m2[5] + m1[2] * m2[9]  + m1[3] * m2[13];
	m[2] = m1[0] * m2[2] + m1[1] * m2[6] + m1[2] * m2[10] + m1[3] * m2[14];
	m[3] = m1[0] * m2[3] + m1[1] * m2[7] + m1[2] * m2[11] + m1[3] * m2[15];

	m[4] = m1[4] * m2[0] + m1[5] * m2[4] + m1[6] * m2[8]  + m1[7] * m2[12];
	m[5] = m1[4] * m2[1] + m1[5] * m2[5] + m1[6] * m2[9]  + m1[7] * m2[13];
	m[6] = m1[4] * m2[2] + m1[5] * m2[6] + m1[6] * m2[10] + m1[7] * m2[14];
	m[7] = m1[4] * m2[3] + m1[5] * m2[7] + m1[6] * m2[11] + m1[7] * m2[15];

	m[8]  = m1[8] * m2[0] + m1[9] * m2[4] + m1[10] * m2[8]  + m1[11] * m2[12];
	m[9]  = m1[8] * m2[1] + m1[9] * m2[5] + m1[10] * m2[9]  + m1[11] * m2[13];
	m[10] = m1[8] * m2[2] + m1[9] * m2[6] + m1[10] * m2[10] + m1[11] * m2[14];
	m[11] = m1[8] * m2[3] + m1[9] * m2[7] + m1[10] * m2[11] + m1[11] * m2[15];

	m[12] = m1[12] * m2[0] + m1[13] * m2[4] + m1[14] * m2[8]  + m1[15] * m2[12];
	m[13] = m1[12] * m2[1] + m1[13] * m2[5] + m1[14] * m2[9]  + m1[15] * m2[13];
	m[14] = m1[12] * m2[2] + m1[13] * m2[6] + m1[14] * m2[10] + m1[15] * m2[14];
	m[15] = m1[12] * m2[3] + m1[13] * m2[7] + m1[14] * m2[11] + m1[15] * m2[15];

	return m;
}

/*
Mat4 Mat4::Inverse() {
  Mat4 ret;
  float det_1;
  float pos = 0;
  float neg = 0;
  float temp;

  temp = f_[0] * f_[5] * f_[10];
  if (temp >= 0)
    pos += temp;
  else
    neg += temp;
  temp = f_[4] * f_[9] * f_[2];
  if (temp >= 0)
    pos += temp;
  else
    neg += temp;
  temp = f_[8] * f_[1] * f_[6];
  if (temp >= 0)
    pos += temp;
  else
    neg += temp;
  temp = -f_[8] * f_[5] * f_[2];
  if (temp >= 0)
    pos += temp;
  else
    neg += temp;
  temp = -f_[4] * f_[1] * f_[10];
  if (temp >= 0)
    pos += temp;
  else
    neg += temp;
  temp = -f_[0] * f_[9] * f_[6];
  if (temp >= 0)
    pos += temp;
  else
    neg += temp;
  det_1 = pos + neg;

  if (det_1 == 0.0) {
    // Error
  } else {
    det_1 = 1.0f / det_1;
    ret.f_[0] = (f_[5] * f_[10] - f_[9] * f_[6]) * det_1;
    ret.f_[1] = -(f_[1] * f_[10] - f_[9] * f_[2]) * det_1;
    ret.f_[2] = (f_[1] * f_[6] - f_[5] * f_[2]) * det_1;
    ret.f_[4] = -(f_[4] * f_[10] - f_[8] * f_[6]) * det_1;
    ret.f_[5] = (f_[0] * f_[10] - f_[8] * f_[2]) * det_1;
    ret.f_[6] = -(f_[0] * f_[6] - f_[4] * f_[2]) * det_1;
    ret.f_[8] = (f_[4] * f_[9] - f_[8] * f_[5]) * det_1;
    ret.f_[9] = -(f_[0] * f_[9] - f_[8] * f_[1]) * det_1;
    ret.f_[10] = (f_[0] * f_[5] - f_[4] * f_[1]) * det_1;

    // Calculate -C * inverse(A)
ret.f_[12] =
-(f_[12] * ret.f_[0] + f_[13] * ret.f_[4] + f_[14] * ret.f_[8]);
ret.f_[13] =
-(f_[12] * ret.f_[1] + f_[13] * ret.f_[5] + f_[14] * ret.f_[9]);
ret.f_[14] =
-(f_[12] * ret.f_[2] + f_[13] * ret.f_[6] + f_[14] * ret.f_[10]);

ret.f_[3] = 0.0f;
ret.f_[7] = 0.0f;
ret.f_[11] = 0.0f;
ret.f_[15] = 1.0f;
}

*this = ret;
return *this;
}
*
 */