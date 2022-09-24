precision mediump float; 

uniform vec3      u_resolution;           // viewport resolution (in pixels
uniform float     u_opacity; 
uniform float     u_time;  

// Start of ShaderToy image shader
// Source: https://www.shadertoy.com/view/Wdcyz7

//full credits to BigWings:
//https://www.youtube.com/watch?v=rvDo9LvfoVE

#define NUM_LAYERS 2. //3.


float Hash21(vec2 p){
   p = fract (p*vec2(123.34,456.21));
   p+= dot(p,p+45.32);
    return fract(p.x*p.y);
    }

mat2 Rot(float a){
    
 float s = sin(a), c= cos(a);
   return mat2(c,-s,s,c); 
}
float star(vec2 uv){
    float d = length(uv);
    float m = .1/d;
    m *= smoothstep(1.,.01,d);
	return m;
}

vec2 GetPos(vec2 id, vec2 offs){
    		float n = Hash21(id+offs);
    return offs+sin(n)*3.;
}

vec3 StarLayer( vec2 uv){	
    vec3 col = vec3(0);
 	vec2 id = floor(uv)-0.5;

    for(int y = -1; y<=1; y++){
  		for(int x = -1; x<=1; x++){
		    vec2 offs = vec2(x,y);
    		float n = Hash21(id+offs);
    		float size = fract(n*545.32);
            
             float star=  star(GetPos(id,offs));
			vec3 color =sin(vec3(1.,0.2,0.9)*fract(n*2345.2)*123.2)*.5+.5;
            color= color*vec3(1.,1.,.5+size);
  		    star*=sin(u_time*2.+n*6.2831)*.5+1.;
 		    col+= star*size*color;

  		}
    }

 return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord.xy-.5*u_resolution.xy)/u_resolution.y;
    uv*=1.;
    float t = u_time*.05;
    float move = t*3.; 
    uv+= vec2(cos(move),sin(move));
    
    uv *= Rot(t);
      vec3 col = vec3(0.);
    
    
    for (float i=0.; i<1.; i+=1./NUM_LAYERS){
        float depth = fract(i+t);
  		float scale = mix(20.,.5, depth);
        float fade = depth*smoothstep(1.,.9,depth);
        col += StarLayer(uv*scale+i*453.2-move)*fade;
    }
 
    
    fragColor = vec4(col,1.0);
}
// End of ShaderToy image shader 

void main()                                                                          
{                                                                                    
    mainImage(gl_FragColor, gl_FragCoord.xy);                                        
}                                                                                    
