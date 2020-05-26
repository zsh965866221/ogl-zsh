#version 300 es
precision highp float;
precision highp int;

in vec2 vTexCoord1;

uniform float uAmount1;
uniform vec2 uCenter1;

uniform float uAmount2;
uniform vec2 uCenter2;

uniform float uFalloff;

uniform float uTextureWidth1;
uniform float uTextureHeight1;

uniform sampler2D uBitmap1;

const float epsilon = 1e-5;

vec2 getDeltaByCenter(vec2 xy, vec2 center, float amount) {
	vec2 uv = xy - (center - 0.5) * 2.0;

	{
		float r = length(uv);
		float theta = atan(uv.y, uv.x);

		/** \brief Reference to https://www.desmos.com/calculator/xeev1ye4qd */

		float b = uFalloff * uFalloff;
		
		float f = r + (amount * (1.0 + 20.0 * sqrt(b)) / r) * (tanh(r * r / b / b / 40.0) / exp(r));
		r = f;

		uv.x = r * cos(theta);
		uv.y = r * sin(theta);
	}

	vec2 nxy = uv + (center - 0.5) * 2.0;
	return nxy - xy;
}

void main() {
	vec2 uTextureSize = vec2(uTextureWidth1, uTextureHeight1);
	float aspect = uTextureSize.y / uTextureSize.x;

	vec2 xy = vTexCoord1 * 2.0 - 1.0;
	xy *= vec2(1.0, aspect);

	vec2 delta = vec2(0.0, 0.0);
	delta += getDeltaByCenter(xy, uCenter1 / uTextureSize, uAmount1);
	delta += getDeltaByCenter(xy, uCenter2 / uTextureSize, uAmount2);

	xy += delta;
	xy = xy / vec2(1.0, aspect);
	xy = (xy + 1.0) / 2.0;

	if (xy.x > 1.0 || xy.y > 1.0 || xy.x < 0.0 || xy.y < 0.0) {
		return;
	}

	
	gl_FragColor = texture2D(uBitmap1, xy);
}