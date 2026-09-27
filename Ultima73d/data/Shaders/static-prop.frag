#version 460 core
in vec3 worldNormal,worldPosition,modelPosition;
in vec2 uv;
flat in int surface;
uniform vec3 eye;
uniform sampler2D baseColorTexture;
uniform int hasBaseColorTexture=0;
// Metres per model unit (the placement scale): patterns keep real-world size.
uniform float unitMetres=1.;
out vec4 FragColor;

// The GLBs have no UVs: every surface pattern is procedural in object space,
// so it sticks to the model and never swims.
float hash(vec3 p){p=fract(p*.3183099+.1);p*=17.;return fract(p.x*p.y*p.z*(p.x+p.y+p.z));}
float noise(vec3 x){
    vec3 i=floor(x),f=fract(x);f=f*f*(3.-2.*f);
    return mix(mix(mix(hash(i),hash(i+vec3(1,0,0)),f.x),mix(hash(i+vec3(0,1,0)),hash(i+vec3(1,1,0)),f.x),f.y),
               mix(mix(hash(i+vec3(0,0,1)),hash(i+vec3(1,0,1)),f.x),mix(hash(i+vec3(0,1,1)),hash(i+vec3(1,1,1)),f.x),f.y),f.z);
}
float fbm(vec3 p){float sum=0.,amp=.5;for(int i=0;i<4;++i){sum+=amp*noise(p);p=p*2.03+vec3(1.7,9.2,3.1);amp*=.5;}return sum;}
// Distance to the nearest cell border (scales, leather pebbles).
float cells(vec3 p){
    vec3 i=floor(p),f=fract(p);float d1=8.,d2=8.;
    for(int z=-1;z<=1;++z)for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x){
        vec3 o=vec3(x,y,z),r=o+vec3(hash(i+o),hash(i+o+11.3),hash(i+o+23.7))-f;float d=dot(r,r);
        if(d<d1){d2=d1;d1=d;}else if(d<d2)d2=d;
    }
    return sqrt(d2)-sqrt(d1);
}
// Fades a pattern of the given frequency (per metre) out before it aliases.
float detail(vec3 m,float frequency){return clamp(1.5-length(fwidth(m))*frequency*2.,0.,1.);}

void main(){
    if(surface==38 && hasBaseColorTexture!=0){
        vec3 texel=texture(baseColorTexture,uv).rgb;
        vec3 n=normalize(worldNormal),light=normalize(vec3(-.6,-.8,1.6));
        float diffuse=.48+.52*abs(dot(n,light));
        float rim=pow(1.-abs(dot(n,normalize(eye-worldPosition))),3.)*.10;
        FragColor=vec4(texel*(diffuse+rim),1.);return;
    }
    vec3 colors[44]=vec3[44](vec3(.63,.39,.28),vec3(.64,.57,.43),vec3(.20,.24,.13),
        vec3(.18,.105,.065),vec3(.075,.040,.025),vec3(.16,.085,.04),vec3(.25,.018,.012),
        vec3(.72,.49,.36),vec3(.20,.13,.095),vec3(.48,.095,.070),vec3(.10,.045,.030),
        vec3(.38,.24,.10),vec3(.30,.004,.006),vec3(.37,.060,.055),
        // stable furnishings in the U7 palette: hay, table planks, iron, tool handles
        vec3(.62,.45,.09),vec3(.33,.24,.19),vec3(.36,.36,.36),vec3(.78,.55,.46),
        // garden set: linen cloth, red stripe, straw highlight, twine binding, oak
        vec3(.80,.72,.60),vec3(.56,.13,.10),vec3(.86,.70,.22),vec3(.42,.28,.12),vec3(.50,.32,.17),
        // horseshoe nail holes; red ritual candle wax, blue flame; brass key
        vec3(.07,.065,.06),vec3(.55,.05,.04),vec3(.35,.55,1.),vec3(.72,.56,.18),
        // glow (drawn apart), chamber tablecloth, sackcloth, gold, red stone
        vec3(.10,.20,.62),vec3(.81,.60,.49),vec3(.50,.37,.30),vec3(.93,.75,.25),vec3(.70,.05,.08),
        // water trough: stone, water
        vec3(.55,.55,.55),vec3(.25,.30,.95),
        // Jolo: aged white hair, brown leather clothing, iron hardware, living skin
        vec3(.82,.80,.72),vec3(.31,.135,.055),vec3(.40,.40,.39),vec3(.58,.31,.20),vec3(1.),
        // Separate fitted leather-set panels: chest, shoulders, trousers and belt.
        vec3(.40,.15,.055),vec3(.31,.105,.038),vec3(.25,.075,.026),vec3(.14,.042,.018),
        vec3(.12,.032,.014));
    // Ritual flame: unlit, pale blue-white core to a deep blue tip.
    // Light pool of a ritual flame (drawn additively): blue, fading out over half a metre.
    if(surface==27){float fall=1.-smoothstep(0.,.5,length(modelPosition.xz));FragColor=vec4(vec3(.10,.20,.62)*fall*fall,1);return;}
    if(surface==25){FragColor=vec4(mix(vec3(.80,.92,1.),vec3(.12,.30,1.),smoothstep(.28,.345,modelPosition.y)),1);return;}
    vec3 n=normalize(worldNormal),light=normalize(vec3(-.6,-.8,1.6));
    float diffuse=.48+.52*abs(dot(n,light));
    float rim=pow(1.-abs(dot(n,normalize(eye-worldPosition))),3.)*.10;
    vec3 color=colors[clamp(surface,0,43)];
    vec3 m=modelPosition*unitMetres;                       // object space in metres
    float grime=fbm(m*9.);                                 // large soft stains, shared by cloth and skin
    if(surface==0 || surface==7 || surface==37){
        // Skin: fine mottling and a dead pallor in the hollows.
        float mottle=fbm(m*55.);
        color*=.90+.16*mottle*detail(m,55.);
        color=mix(color,vec3(.55,.45,.40),smoothstep(.55,.8,grime)*.25);
    }
    if(surface==37){
        // Jolo's living, weathered skin. His source has no UVs, so facial
        // details are anchored in model space on the forward (+Z) head side.
        float pores=hash(floor(m*1100.));
        float fine=fbm(m*180.);
        color*=.88+.13*mix(.5,pores,detail(m,1100.))+.08*fine;
        float head=smoothstep(.232,.244,modelPosition.y);
        float front=smoothstep(-.004,.012,modelPosition.z)*head;
        vec2 face=vec2(modelPosition.x,modelPosition.y);
        float eyeY=.274;
        float eyeShape=1.-smoothstep(.0021,.0042,length(vec2(abs(face.x)-.0078,(face.y-eyeY)*1.55)));
        float iris=1.-smoothstep(.0008,.0019,length(vec2(abs(face.x)-.0078,(face.y-eyeY)*1.7)));
        float pupil=1.-smoothstep(.00025,.00075,length(vec2(abs(face.x)-.0078,(face.y-eyeY)*1.7)));
        float brow=(1.-smoothstep(.0012,.0025,abs(face.y-.2815)))*(1.-smoothstep(.006,.012,abs(face.x)));
        float nose=(1.-smoothstep(.0012,.0032,abs(face.x)))*smoothstep(.255,.263,face.y)*(1.-smoothstep(.271,.276,face.y));
        float nostril=(1.-smoothstep(.001,.0023,abs(abs(face.x)-.0024)))*(1.-smoothstep(.001,.002,abs(face.y-.258)));
        float mouthPatch=1.-smoothstep(.82,1.08,length(vec2(face.x/.0075,(face.y-.2585)/.0045)));
        float mouth=(1.-smoothstep(.00055,.00135,abs(face.y-.2588)))*(1.-smoothstep(.0042,.0068,abs(face.x)));
        float lowerLip=(1.-smoothstep(.0006,.0015,abs(face.y-.2573)))*(1.-smoothstep(.0035,.0058,abs(face.x)));
        float moustache=(1.-smoothstep(.0008,.0022,abs(face.y-.2630)))*
                        smoothstep(.0008,.0022,abs(face.x))*(1.-smoothstep(.0065,.0100,abs(face.x)));
        float beardOval=1.-smoothstep(.82,1.08,length(vec2(face.x/.015,(face.y-.245)/.017)));
        float sideBeard=smoothstep(.008,.012,abs(face.x))*(1.-smoothstep(.015,.020,abs(face.x)))*
                        smoothstep(.244,.254,face.y)*(1.-smoothstep(.266,.276,face.y));
        float beard=clamp(beardOval+sideBeard,0.,1.)*front;
        float beardStrands=.72+.28*(.5+.5*sin(face.x*2900.+face.y*1700.));
        float cheeks=smoothstep(.005,.010,abs(face.x))*(1.-smoothstep(.010,.015,abs(face.x)))*
                     smoothstep(.252,.261,face.y)*(1.-smoothstep(.266,.273,face.y));
        color=mix(color,vec3(.67,.34,.23),cheeks*front*.30);
        color=mix(color,vec3(.76,.69,.57),eyeShape*front*.92);
        color=mix(color,vec3(.22,.34,.26),iris*front*.90);
        color=mix(color,vec3(.018,.012,.009),pupil*front);
        color=mix(color,vec3(.34,.30,.25),brow*front*.75);
        color=mix(color,mix(vec3(.58,.57,.54),vec3(.93,.92,.86),beardStrands),beard*.94);
        color=mix(color,vec3(.59,.31,.21),mouthPatch*front*.96);
        color=mix(color,vec3(.70,.39,.27),nose*front*.20);
        color=mix(color,vec3(.20,.055,.040),nostril*front*.80);
        color=mix(color,vec3(.91,.90,.85),moustache*front*.98);
        color=mix(color,vec3(.48,.018,.022),mouth*front*.96);
        color=mix(color,vec3(.78,.10,.10),lowerLip*front*.72);
    }else if(surface==1){
        // Linen shirt: plain weave, soft folds, dirt and dried blood towards the wound.
        float warp=.5+.5*sin(m.x*520.),weft=.5+.5*sin(m.z*520.+m.y*140.);
        float weave=mix(.5,warp*weft+.25,detail(m,520.));
        float folds=fbm(m*vec3(6,14,4));
        color*=.80+.16*weave+.18*folds;
        color=mix(color,vec3(.42,.36,.26),smoothstep(.5,.78,grime)*.55);
        color=mix(color,vec3(.33,.03,.02),smoothstep(.62,.82,fbm(m*14.+3.))*.55);
    }else if(surface==2){
        // Trousers: diagonal twill, worn lighter on knees and seat, mud at the hems.
        float twill=.5+.5*sin((m.x+m.z)*700.+m.y*200.);
        color*=.84+.18*mix(.5,twill,detail(m,700.))+.14*fbm(m*vec3(5,12,5));
        color=mix(color,color*1.45,smoothstep(.62,.86,fbm(m*5.+7.))*.6);
        color=mix(color,vec3(.17,.11,.06),smoothstep(.55,.8,grime)*.6);
    }else if(surface==3 || surface==5){
        // Boots and belt: pebbled leather, creases and scuffed highlights.
        float pebble=cells(m*260.);
        float grain=mix(.5,smoothstep(0.,.25,pebble),detail(m,260.));
        float creases=smoothstep(.35,.0,abs(noise(m*vec3(40,8,40))-.5));
        color*=.72+.36*grain-.22*creases*detail(m,40.);
        color=mix(color,color*1.9,smoothstep(.70,.9,fbm(m*18.))*.5);
    }else if(surface==6){
        // Soaked shirt around the wound: uneven, darker where the blood pooled.
        color*=.72+.45*fbm(m*28.);
    }else if(surface==4){
        // Layered, irregular hair strands instead of a flat brown cap.
        float around=atan(modelPosition.x,modelPosition.z-.405);
        float strands=.5+.5*sin(around*17.+modelPosition.y*145.);
        float fine=.5+.5*sin(around*31.-modelPosition.z*173.+modelPosition.x*67.);
        float roots=smoothstep(.07,.14,modelPosition.y);
        vec3 dark=vec3(.038,.019,.011),warm=vec3(.115,.058,.030),tip=vec3(.18,.095,.048);
        color=mix(dark,warm,.24+.50*strands);
        color=mix(color,tip,fine*.20*(1.-roots*.45));
    }
    if(surface==7){
        // Readable face features in object space (no UVs in the source).
        float eyeX=abs(abs(modelPosition.x)-.038),eyeZ=abs(modelPosition.z-.435);
        float eyes=1.-smoothstep(.006,.016,max(eyeX,eyeZ));
        float nose=1.-smoothstep(.010,.030,abs(modelPosition.x));
        float mouth=(1.-smoothstep(.005,.013,abs(modelPosition.z-.397)))*
                    (1.-smoothstep(.045,.075,abs(modelPosition.x)));
        float cheeks=smoothstep(.025,.075,abs(modelPosition.x))*(1.-smoothstep(.075,.12,abs(modelPosition.x)));
        color=mix(color,vec3(.69,.40,.31),cheeks*.18);
        color=mix(color,vec3(.70,.43,.31),nose*.20);
        color=mix(color,vec3(.025,.018,.014),eyes*.88);
        color=mix(color,vec3(.20,.055,.045),mouth*.62);
    }else if(surface==8){
        float stubble=.78+.22*hash(floor(m*900.))*detail(m,900.);
        color*=stubble;
    }else if(surface==34){
        // Jolo's white hair: fine silver strands with soft, darker roots.
        float strands=.5+.5*sin((m.x+m.z)*430.+m.y*115.);
        float fine=.5+.5*sin((m.x-m.z)*710.-m.y*83.);
        color*=.80+.23*mix(.5,strands*.7+fine*.3,detail(m,430.));
        color=mix(vec3(.48,.46,.42),color,smoothstep(.04,.19,modelPosition.y));
    }else if(surface==35 || (surface>=39 && surface<=43)){
        // Jolo's fitted leather: pebbled grain, creases, wear and dark seams.
        float pebble=cells(m*230.);
        float grain=mix(.5,smoothstep(0.,.24,pebble),detail(m,230.));
        float creases=fbm(m*vec3(28.,9.,28.));
        color*=.67+.30*grain+.20*creases;
        color=mix(color,vec3(.12,.045,.018),smoothstep(.62,.84,grime)*.48);
        color=mix(color,vec3(.53,.28,.11),smoothstep(.72,.90,fbm(m*35.+4.))*.30);
    }else if(surface==36){
        // Iron rivets and buckles: worn bright faces with darker recessed edges.
        float dents=cells(m*190.);
        color*=.73+.32*mix(.5,smoothstep(0.,.28,dents),detail(m,190.));
        float shine=pow(max(dot(reflect(-normalize(eye-worldPosition),n),light),0.),20.);
        color+=vec3(.34)*shine;
    }else if(surface==9){
        // Gargoyle hide: small overlapping scales, darker creases, mottled red.
        float scale=cells(m*110.);
        float edge=smoothstep(.0,.18,scale);
        color*=mix(1.,.62+.45*edge,detail(m,110.));
        color*=.84+.30*fbm(m*16.);
        color=mix(color,vec3(.20,.03,.02),smoothstep(.62,.85,grime)*.45);
    }else if(surface==10){
        // Horn and claws: growth rings along the length, pale worn tips.
        float rings=.5+.5*sin(m.y*420.+fbm(m*30.)*6.);
        color*=.72+.38*mix(.5,rings,detail(m,420.));
        color=mix(color,vec3(.42,.36,.28),smoothstep(.55,.85,fbm(m*20.+5.))*.35);
    }else if(surface==11){
        // Loincloth: coarse homespun weave, frayed dark hem and dirt.
        float warp=.5+.5*sin(m.x*300.),weft=.5+.5*sin(m.y*300.);
        color*=.78+.30*mix(.5,warp*weft+.2,detail(m,300.))+.14*fbm(m*12.);
        color=mix(color,vec3(.14,.09,.04),smoothstep(.5,.8,grime)*.6);
    }else if(surface==12){
        float wet=.78+.22*pow(max(dot(n,normalize(eye-worldPosition)),0.),12.);
        color*=wet*(.85+.25*fbm(m*40.));
    }else if(surface==13){
        // Wing membrane: branching dark veins over a thinner, lighter skin.
        float veins=pow(1.-abs(fbm(m*vec3(9,26,9))*2.-1.),10.);
        color=mix(color*1.18,vec3(.16,.02,.02),veins*.75);
        color*=.88+.20*fbm(m*40.)*detail(m,40.);
    }
    if(surface==14 || surface==20){
        // Hay: bright golden strands over dark gaps (U7 4c2400, 895508, c69518, eeca28).
        vec3 q=m*vec3(1,1.6,1);
        float strands=0.;
        for(int k=0;k<3;++k){
            vec3 direction=normalize(vec3(sin(k*2.1+.4),.35*cos(k*1.7),cos(k*2.1+.4)));
            float t=dot(q,direction)*42.+fbm(q*5.+float(k)*7.)*5.;
            strands=max(strands,smoothstep(.62,.95,abs(fract(t)-.5)*2.));
        }
        strands*=detail(m,42.);
        float body=fbm(m*14.);
        color=mix(vec3(.62,.42,.12),vec3(.84,.64,.24),smoothstep(.2,.65,body));
        color=mix(color,vec3(.96,.80,.38),strands*.85);
        color=mix(color,vec3(1.,.90,.55),strands*smoothstep(.55,.8,fbm(m*30.+3.))*.8);
        if(surface==20)color=mix(color,vec3(1.,.88,.45),.45);
    }else if(surface==15 || surface==17 || surface==22){
        // Wood: grain along the length (object x), knots, darker plank edges.
        float along=m.x*(surface==15?3.:6.);
        float grain=.5+.5*sin((m.y+m.z)*(surface==15?260.:420.)+fbm(vec3(along,m.y*9.,m.z*9.))*7.);
        float knots=smoothstep(.72,.9,fbm(m*vec3(4,26,26)));
        color*=.80+.24*mix(.5,grain,detail(m,420.))-.25*knots;
        if(surface==15)color=mix(color,vec3(.12,.08,.06),smoothstep(.55,.8,grime)*.35);
    }else if(surface==18 || surface==19){
        // Table linen: plain weave with soft creases.
        float warp=.5+.5*sin(m.x*380.),weft=.5+.5*sin(m.z*380.);
        color*=.86+.16*mix(.5,warp*weft+.25,detail(m,380.))+.10*fbm(m*vec3(8,20,8));
    }else if(surface==21){
        // Twisted twine binding.
        color*=.75+.35*(.5+.5*sin((m.x+m.y+m.z)*600.))*detail(m,600.);
    }else if(surface==28){
        // U7 tablecloth: fine basket weave in two rose-beige tones (ce997d, be815d).
        float cell=mod(floor(m.x*90.)+floor(m.z*90.),2.);
        float weave=mix(.5,cell,detail(m,90.));
        color=mix(vec3(.75,.51,.36),vec3(.81,.60,.49),weave)*(.92+.10*fbm(m*vec3(6,20,6)));
    }else if(surface==29){
        // Sackcloth: coarse weave, darker in the folds.
        float warp=.5+.5*sin(m.x*420.),weft=.5+.5*sin(m.y*420.);
        color*=.80+.25*mix(.5,warp*weft+.2,detail(m,420.))+.12*fbm(m*vec3(10,20,10));
    }else if(surface==30 || surface==31){
        // Gold and the red stone: bright specular highlights.
        float shine=pow(max(dot(reflect(-normalize(eye-worldPosition),n),light),0.),surface==31?40.:18.);
        color=color*(.85+.2*fbm(m*120.))+vec3(1.,.9,.6)*shine*(surface==31?.6:.45);
    }else if(surface==32){
        // Trough stone: dressed blocks with dark joints, lighter worn edges.
        float blocks=cells(m*vec3(3.,6.,3.));
        color*=.78+.30*fbm(m*25.)-.25*(1.-smoothstep(0.,.06,blocks));
    }else if(surface==33){
        // Still water: U7 blue with a bright sky glint.
        float glint=pow(max(dot(reflect(-normalize(eye-worldPosition),n),light),0.),30.);
        color=mix(color,vec3(.35,.45,1.),.3*fbm(m*8.))+vec3(.7,.8,1.)*glint*.6;
    }else if(surface==24){
        // Red ritual wax: soft, slightly translucent towards the lit side.
        color*=.88+.18*fbm(m*60.);color+=vec3(.10,.02,.02)*pow(max(dot(n,light),0.),2.);
    }else if(surface==26){
        // Brass: warm, worn bright on the edges.
        color*=.85+.25*fbm(m*90.);color+=vec3(.20,.16,.06)*pow(max(dot(reflect(-normalize(eye-worldPosition),n),light),0.),12.);
    }else if(surface==16){
        // Forged iron: hammer dents and brown rust in patches.
        float dents=cells(m*180.);
        color*=.78+.35*mix(.5,smoothstep(0.,.3,dents),detail(m,180.));
        color=mix(color,vec3(.33,.17,.08),smoothstep(.55,.8,fbm(m*25.+2.))*.55);
        color+=vec3(.10)*pow(max(dot(reflect(-normalize(eye-worldPosition),n),light),0.),16.);
    }
    // Ultima73d: fresh blood on the corpse (surfaces 0-8) and the gargoyle (9-13), splashes and
    // streaks running down in object space, glossy where it is wet.
    if(surface<=13){
        float splash=fbm(m*7.+vec3(3.1,0.,7.7))*.65+fbm(m*23.)*.35;
        float runs=smoothstep(.55,.9,fbm(m*vec3(30.,4.,30.)+11.));
        float amount=clamp((smoothstep(.48,.60,splash)+runs*.6)*(surface<=8?1.:.85),0.,1.);
        vec3 blood=mix(vec3(.32,.004,.006),vec3(.11,0.,0.),smoothstep(.6,.9,fbm(m*40.)));
        color=mix(color,blood,amount*.9);
        color+=vec3(.5,.18,.18)*pow(max(dot(reflect(-normalize(eye-worldPosition),n),light),0.),40.)*amount;
    }
    // Ultima73d: fresh blood on the corpse (surfaces 0-8) and the gargoyle (9-13): heavily
    // soaked, large splashes and runs, most on the chest and belly, glossy where it is wet.
    if(surface<=13){
        float splash=fbm(m*7.+vec3(3.1,0.,7.7))*.65+fbm(m*23.)*.35;
        float runs=smoothstep(.55,.9,fbm(m*vec3(30.,4.,30.)+11.));
        float body=surface<=8?smoothstep(-.05,.30,m.z)*.35:smoothstep(.6,1.2,m.y)*.35;
        float amount=clamp((smoothstep(.40,.54,splash)+runs*.85+body)*(surface<=8?1.:.95),0.,1.);
        vec3 blood=mix(vec3(.32,.004,.006),vec3(.11,0.,0.),smoothstep(.6,.9,fbm(m*40.)));
        color=mix(color,blood,amount*.95);
        color+=vec3(.5,.18,.18)*pow(max(dot(reflect(-normalize(eye-worldPosition),n),light),0.),40.)*amount;
    }
    color*=diffuse+rim;
    FragColor=vec4(pow(color,vec3(1./1.08)),1);
}
