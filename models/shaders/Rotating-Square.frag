precision mediump float;                                                                        
                                                                                                
uniform vec3      u_resolution;           // viewport resolution (in pixels                     
uniform float     u_opacity;                                                                    
uniform float     u_time;                                                                       
                                                                                                
vec2 rotate(vec2 uv, float th) {                                                                
  return mat2(cos(th), sin(th), -sin(th), cos(th)) * uv;                                        
}                                                                                               
                                                                                                
vec3 sdfSquare(vec2 uv, float size, vec2 offset) {                                              
  float x = uv.x - offset.x;                                                                    
  float y = uv.y - offset.y;                                                                    
  vec2 rotated = rotate(vec2(x,y), u_time);                                                     
  float d = max(abs(rotated.x), abs(rotated.y)) - size;                                         
                                                                                                
  return d > 0. ? vec3(1.) : vec3(1., 0., 0.);                                                  
}                                                                                               
                                                                                                
void mainImage( out vec4 fragColor, in vec2 fragCoord )                                         
{                                                                                               
  vec2 uv = fragCoord/u_resolution.xy; // <0, 1>                                                
  uv -= 0.5; // <-0.5,0.5>                                                                      
  uv.x *= u_resolution.x/u_resolution.y; // fix aspect ratio                                    
                                                                                                
  vec2 offset = vec2(0.0, 0.0);                                                                 
                                                                                                
  vec3 col = sdfSquare(uv, 0.2, offset);                                                        
                                                                                                
  // Output to screen                                                                           
  fragColor = vec4(col,1.0);                                                                    
}                                                                                               
                                                                                                
void main()                                                                                     
{                                                                                               
    mainImage(gl_FragColor, gl_FragCoord.xy);                                                   
}   