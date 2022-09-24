
#version 300 es

precision mediump float;
                                                                     
uniform vec3      u_resolution;           // viewport resolution (in pixels                     
uniform float     u_opacity;                                                                    
uniform float     u_time;    

// output
out vec4 outColor; 

//Source https://www.shadertoy.com/view/lddcR8

mat4 translationMatrix(vec3 v) {
	return mat4(1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                v.x, v.y, v.z, 1.0);		
}

mat4 rotateX(float rad) {
	return mat4(1.0, 0.0, 0.0, 0.0,
                0.0, cos(rad), sin(rad), 0.0,
                0.0, -sin(rad), cos(rad), 0.0,
				0.0, 0.0, 0.0, 1.0);
}

mat4 rotateY(float rad) {
	return mat4(cos(rad), 0.0, -sin(rad), 0.0,
                0.0, 1.0, 0.0, 0.0,
                sin(rad), 0.0, cos(rad), 0.0,
				0.0, 0.0, 0.0, 1.0);
}


mat2 rotateM(float rad) {
    return mat2(cos(rad), sin(rad),
                 -sin(rad), cos(rad));
}

float ease_in_quadratic(float t) {
    return 0.0;
}

float ease_in_out_quadratic(float t) {
    if (t < 0.5) {
        return ease_in_quadratic(t * 2.0) / 2.0;
    } else {
        return 1.0 - ease_in_quadratic((1.0 - t) * 2.0) / 2.0;
    }
        
}

float udBox( vec3 p, vec3 b )
{
    return length(max(abs(p)-b,0.0));
}

float sceneSDF(vec3 samplePoint) {
    return udBox(samplePoint, vec3(1.0, 1.0, 1.0));
}

vec3 rayDirection(vec2 fragCoord, vec3 u_resolution) {
    return vec3(0.0, 0.0, -1.0);
}


float march(vec3 eyePos, vec3 rDir, float min_t, float max_t, float EPSILON, mat4 transM) {
    const int MAX_MARCHING_STEPS = 255;

    float t = min_t;
    for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
        vec3 p = eyePos + t * rDir;
        
        float PI = 3.14159265359;
        vec3 transedP = vec3(inverse(transM) * vec4(p, 1.0));
        float dist = sceneSDF(transedP);
        if (dist < EPSILON) {
            return t;
        }
        t += dist;

        if (t >= max_t) {
            return max_t;
        }
    }
    return max_t;
}

vec3 estimateNormal(vec3 p) {
    const float EPSILON = 0.0001;
    return normalize(vec3(
        sceneSDF(vec3(p.x + EPSILON, p.y, p.z)) - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)),
        sceneSDF(vec3(p.x, p.y + EPSILON, p.z)) - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)),
        sceneSDF(vec3(p.x, p.y, p.z  + EPSILON)) - sceneSDF(vec3(p.x, p.y, p.z - EPSILON))
    ));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float modTime = mod(u_time, 4.0);
 	int mode = 0;   
 
    if (modTime > 0.0 && modTime <= 1.0) {
        mode = 1;
    } else if (modTime > 1.0 && modTime <= 2.0) {
        mode = 2;
    } else if (modTime > 2.0 && modTime <= 3.0) {
        mode = 3;
    } else if(modTime > 3.0 && modTime <= 4.0) {
        mode = 4;
    }


	const float MIN_T = 0.0;
	const float MAX_T = 100.0;
	const float EPSILON = 0.0001;
    vec3 eyePos = vec3(0.0, 0.0, 80.0);
    eyePos.x = fragCoord.x / 50.0 - 0.5 * u_resolution.x / 50.0;
    eyePos.y = fragCoord.y / 50.0 - 0.5 * u_resolution.y / 50.0;

	vec3 rDir = rayDirection(fragCoord, u_resolution);
	vec3 rDir_transed = rDir;
    
    float PI = 3.14159265359;
    mat4 transM = rotateX(0.8 * PI/4.0) * rotateY(PI/4.0);
    if (mode == 1) {
        transM = transM * rotateY( -1.0 * PI * 0.5 * fract(u_time));
    } else if (mode == 3) {
        transM = transM * rotateX(1.0 * PI * 0.5 * fract(u_time));
    }
        
        

    float t = march(eyePos, rDir_transed, MIN_T, MAX_T, EPSILON, transM);
    vec3 pos = eyePos + t * rDir_transed;
    
    pos = vec3(inverse(transM) * vec4(pos, 1.0));
	vec3 normal = estimateNormal(pos);
    
        
    if (t >= MAX_T) {
        fragColor = vec4(0.9, 0.9, 0.9, 1.0);

    } else {
        vec4 purple = vec4(81.0/255.0, 65.0/255.0, 86.0/255.0, 1.0);
        vec4 green = vec4(169.0/255.0, 217.0/255.0, 198.0/255.0, 1.0);
        vec4 pink = vec4(232.0/255.0, 78.0/255.0,128.0/255.0, 1.0);
        vec4 red = vec4(1.0, 0.0, 0.0, 1.0);
        vec2 midP = u_resolution.xy / 2.0;
        
        if (mode == 1) {   
            if (normal.x == 0.0 && normal.y == 0.0 && normal.z > 0.0) {
                // front
                fragColor = purple;
            } else if (normal.x > 0.0 && normal.y == 0.0 && normal.z == 0.0){
                // right
                fragColor = pink;
            } else if (normal.x == 0.0 && normal.y > 0.0 && normal.z == 0.0){
                // top
                fragColor = green;
            } else if (normal.x < 0.0 && normal.y == 0.0 && normal.z == 0.0){
                // left
                fragColor = pink;
            } 
        } else if (mode == 3) {    
            if (normal.x == 0.0 && normal.y == 0.0 && normal.z > 0.0) {
                // front
                fragColor = purple;
            } else if (normal.x == 0.0 && normal.y > 0.0 && normal.z == 0.0){
                // top
                fragColor = pink;
            } else if (normal.x < 0.0 && normal.y == 0.0 && normal.z == 0.0){
                // top
                fragColor = green;
            } else if (normal.x == 0.0 && normal.y == 0.0 && normal.z < 0.0){
                // back
                fragColor = purple;
            } 
            
        } else if (mode == 2) {
            float angle =  (-1.0) * smoothstep(0.0, 1.0, fract(u_time)) * 2.0 * PI / 3.0; 
            vec2 fragCoordRot = rotateM(angle) * (fragCoord - midP) + midP;
            vec2 frag_v = normalize(fragCoordRot - midP);
            vec2 comp_v = vec2(0.0, -1.0);
            float theta = acos(dot(frag_v, comp_v) / 1.0);
            if (theta > 0.0 && theta <= (2.0 * PI) / 3.0) {
                if (fragCoordRot.x >= midP.x) {
                    fragColor = pink;
                } else {
                    fragColor = purple;
                }    
            } else if (theta > (2.0 * PI) / 3.0 && theta <= (4.0 * PI) / 3.0) {
                fragColor = green;
            }
                
            
        } else if (mode == 4) {
            float angle =  smoothstep(0.0, 1.0, fract(u_time)) * 2.0 * PI / 3.0; 
            vec2 fragCoordRot = rotateM(angle) * (fragCoord - midP) + midP;
            vec2 frag_v = normalize(fragCoordRot - midP);
            vec2 comp_v = vec2(0.0, -1.0);
            float theta = acos(dot(frag_v, comp_v) / 1.0);
            if (theta > 0.0 && theta <= (2.0 * PI) / 3.0) {
                if (fragCoordRot.x >= midP.x) {
                    fragColor = pink;
                } else {
                    fragColor = green;
                }    
            } else if (theta > (2.0 * PI) / 3.0 && theta <= (4.0 * PI) / 3.0) {
                fragColor = purple;
            }
        }
	}
}

void main()                                                                                     
{                                                                                               
    mainImage(outColor, gl_FragCoord.xy);                                                   
}   
