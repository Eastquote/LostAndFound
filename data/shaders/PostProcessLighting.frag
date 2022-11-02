#version 120

uniform sampler2D fgLightMaskTexture;
uniform sampler2D bkgLightMaskTexture;
uniform sampler2D lightsTexture;
uniform sampler2D skyboxTexture;
uniform sampler2D parallaxTexture;
uniform sampler2D bkgTilesTexture;
uniform sampler2D texture; // backgroundGameplay
uniform sampler2D fgTilesTexture;
uniform sampler2D fgGameplayTexture;
uniform sampler2D hudTexture;


void main()
{
	vec4 fgLightMaskTexturePixel = texture2D(fgLightMaskTexture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
	vec4 bkgLightMaskTexturePixel = texture2D(bkgLightMaskTexture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
	vec4 lightsTexturePixel = texture2D(lightsTexture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
	vec4 skyboxTexturePixel = texture2D(skyboxTexture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
	vec4 parallaxTexturePixel = texture2D(parallaxTexture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
	vec4 bkgTilesTexturePixel = texture2D(bkgTilesTexture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
	vec4 texturePixel = texture2D(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
	vec4 fgTilesTexturePixel = texture2D(fgTilesTexture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
	vec4 fgGameplayTexturePixel = texture2D(fgGameplayTexture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
	vec4 hudTexturePixel = texture2D(hudTexture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));

	bkgLightMaskTexturePixel = bkgLightMaskTexturePixel * vec4(1, 1, 1, (1.0 - fgTilesTexturePixel.a));
	fgLightMaskTexturePixel = fgLightMaskTexturePixel * vec4(1, 1, 1, (1.0 - bkgTilesTexturePixel.a));

	vec4 gameplayLightsTexturePixel = lightsTexturePixel;
	if(gameplayLightsTexturePixel.rgb == vec3(139.0 / 255.0, 155.0 / 255.0, 180.0 / 255.0)){
		// set "penumbra" color that gets multiplied down on edge of light:
		gameplayLightsTexturePixel = vec4(139.0 / 255.0, 155.0 / 255.0, 180.0 / 255.0, 1.0);
	}
	if(gameplayLightsTexturePixel.rgb == vec3(0, 0, 0)){
		// set "shadow" color that gets multiplied down on unlit areas:
		gameplayLightsTexturePixel = vec4(83.0 / 255.0, 88.0 / 255.0, 141.0 / 255.0, 1.0);
	}

	vec4 fgLightsTexturePixel = lightsTexturePixel * fgLightMaskTexturePixel;
	if(fgLightsTexturePixel.rgb == vec3(139.0 / 255.0, 155.0 / 255.0, 180.0 / 255.0)){
		// set "penumbra" color that gets multiplied down on edge of light:
		fgLightsTexturePixel = vec4(139.0 / 255.0, 155.0 / 255.0, 180.0 / 255.0, 1.0);
	}
	if(fgLightsTexturePixel.rgb == vec3(0, 0, 0)){
		// set "shadow" color that gets multiplied down on unlit areas:
		fgLightsTexturePixel = vec4(83.0 / 255.0, 88.0 / 255.0, 141.0 / 255.0, 1.0);
	}

	vec4 bkgLightsTexturePixel = lightsTexturePixel * bkgLightMaskTexturePixel;
	if(bkgLightsTexturePixel.rgb == vec3(139.0 / 255.0, 155.0 / 255.0, 180.0 / 255.0)){
		// set "penumbra" color that gets multiplied down on edge of light:
		bkgLightsTexturePixel = vec4(139.0 / 255.0, 155.0 / 255.0, 180.0 / 255.0, 1.0);
	}
	if(bkgLightsTexturePixel.rgb == vec3(0, 0, 0)){
		// set "shadow" color that gets multiplied down on unlit areas:
		bkgLightsTexturePixel = vec4(83.0 / 255.0, 88.0 / 255.0, 141.0 / 255.0, 1.0);
	}

	//lightsTexturePixel.a = lightsTexturePixel.a * fgGameplayTexturePixel.a;

	vec3 fragColor = skyboxTexturePixel.rgb;
	fragColor = mix(fragColor.rgb, parallaxTexturePixel.rgb, parallaxTexturePixel.a);
	fragColor = mix(fragColor.rgb, bkgTilesTexturePixel.rgb * bkgLightsTexturePixel.rgb, bkgTilesTexturePixel.a);
	fragColor = mix(fragColor.rgb, bkgTilesTexturePixel.rgb, fgTilesTexturePixel.a);
	//fragColor = mix(fragColor.rgb, texturePixel.rgb, texturePixel.a);
	fragColor = mix(fragColor.rgb, fgGameplayTexturePixel.rgb * gameplayLightsTexturePixel.rgb, fgGameplayTexturePixel.a);
	fragColor = mix(fragColor.rgb, fgTilesTexturePixel.rgb * fgLightsTexturePixel.rgb, fgTilesTexturePixel.a);
	fragColor = mix(fragColor.rgb, hudTexturePixel.rgb, hudTexturePixel.a);

	gl_FragColor = vec4(fragColor, 1.0);
	//gl_FragColor = bkgTilesTexturePixel;
}
