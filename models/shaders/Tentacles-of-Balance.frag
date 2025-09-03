precision mediump float; 

uniform vec3      u_resolution;           // viewport resolution (in pixels
uniform float     u_opacity; 
uniform float     u_time;  

// Start of ShaderToy image shader
// Source: https://www.shadertoy.com/view/sdBcWc
#define pi 3.14159265

// Function to calculate the color gradient. Mostly arbitrary. Polynomials would probably be faster.
// Values are case-specifc & arbitrary.
vec3 grad(float t){
    float r = sin(t-pi)+0.1*sin(u_time);
    float g = sin(t+0.25*pi);
    float b = sin(t+0.5*pi);
    vec3 color = vec3(r,g,b);
    return color;
}

// Creates the tentacle with the position & outputs it's color contribution.
vec3 tent(float a,vec2 p,float end){
    
    float y = (0.5+(0.25*sin(a*5.0*p.x+0.5*u_time)));
    float x = p.x+0.01*sin(0.1*u_time);
    
    vec2 point = vec2(x,y);
    float dist = distance(p,point);
    
    vec3 color = grad(5.0*point.x+0.5);
    if (dist!=0.0) {color *= abs(0.03/dist);}
    return max(color,vec3(0,0,0));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/u_resolution.xy;

    // Changes sign half way across the x-axis.
    float pos_sign = 1.0;
    if (uv.x<0.5) {pos_sign *= -1.0;}
    
    // Both color calculations. 
    vec3 shift = vec3(0.5)*sin(u_time)+pos_sign; // Pulsating 
    vec3 adjusted = grad(5.0*uv.x+0.5)*0.5*shift*shift; // Red & Green glow

    // Four additions for each tentacle's color contribution.
    vec3 color = abs(tent(pos_sign*1.0,uv,2.0));
    color += abs(tent(1.16,uv-vec2(0.13,0.15),15.0));
    color += abs(tent(0.25,uv,7.0));
    color += abs(tent(pos_sign*1.7,uv,20.0))*vec3(2.0,2.0,0.7);

    // Output to screen
    fragColor = vec4(color*adjusted,1.0);
}
// End of ShaderToy image shader 

void main()                                                                          
{                                                                                    
    mainImage(gl_FragColor, gl_FragCoord.xy);                                        
}                                                                                    
