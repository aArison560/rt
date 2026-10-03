#include "core/Scene.hpp"
#include "core/Object.hpp"
#include "core/Texture.hpp" // tex image
#include <cstdio>
#include <cstring>
#include <string>

/* "x,y,z" -> Vec3. */
static bool	parseVec3(const char* s, Vec3& out){
	return sscanf(s, "%lf,%lf,%lf", &out.x, &out.y, &out.z) == 3;
}
/* "r,g,b" -> Vec3 0-255. */
static bool	parseColor(const char* s, Vec3& out){
	int	r,
		g,
		b;
	
	if (sscanf(s, "%d,%d,%d", &r, &g, &b) != 3) return false;
	out=Vec3(r, g ,b); return true;
}

/* Parse le .rt ligne par ligne. 0 = ok, 1 = erreur (ligne loggée, fichier fermé). */
int	parse_scene(const char* path, Scene* scene){
	if (!path || !scene) return 1;

	FILE*	f = fopen(path,"r");
	
	if (!f)
		{ fprintf(stderr,"Error: cannot open '%s'\n",path); return 1; }

	char	line[2048];
	int		no = 0;
	
	scene->clear(); // reset avant recharge (anti-doublons)
	scene->filepath = path;
	scene->cam = Camera(); 
	scene->light = Light();
	scene->ambient = Vec3(0.1, 0.1, 0.1);
	scene->width = 1080;
	scene->height = 720;
	while (fgets(line, sizeof(line), f)){
		no++;
		char* p = line;
		while (*p == ' ' || *p == '\t') p++;
		if (*p == '\n' || *p == '\0'|| *p == '#') continue;
		char*	nl = strchr(p, '\n'); 
		if (nl) *nl = '\0';
		nl = strchr(p,'\r'); if(nl) *nl='\0';
		if (*p == '\0') continue;
		if (strncmp(p, "C ", 2) == 0){ // caméra : C pos dir fov
			char	sp[64],
					sd[64];
			double	fov;
			if (sscanf(p+2, "%63s %63s %lf", sp, sd, &fov) != 3) { 
				fprintf(stderr, "Error line %d: C\n", no); 
				fclose(f); 
				return 1; 
			}

			Vec3	pos,
					dir;
			
			if (!parseVec3(sp, pos) || !parseVec3(sd,dir)){ fclose(f); return 1; }
			scene->cam.pos = pos;
			scene->cam.dir = dir.normalized();
			scene->cam.fov = fov;
		} else if (strncmp(p,"L ", 2) == 0){ // lumière : L pos intensité
			char sp[64];
			double intens;
			if(sscanf(p+2, "%63s %lf", sp, &intens) != 2){ fclose(f); return 1; }
			Vec3	pos;
			
			parseVec3(sp, pos); 
			scene->light.pos = pos;
			scene->light.intensity = intens;
		} else if (strncmp(p, "sp ", 3) == 0) { // sphère : sp pos radius color
			char	sp[64],
					sc[64];
			double	rad;
			if (sscanf(p+3, "%63s %lf %63s", sp, &rad, sc) != 3){ fclose(f); return 1; }
			Vec3	pos,
					col; 
			
			parseVec3(sp, pos);
			parseColor(sc, col);
			scene->objects.push_back(std::make_unique<Sphere>(pos, rad, col));
			
			// miroir liste C (ancien code)
			s_obj* o = new s_obj{OBJ_SPHERE, pos, Vec3(), rad, 0, col, nullptr};
			o->next = scene->objs;
			scene->objs = o;
		} else if (strncmp(p, "pl ", 3) == 0){ // plan : pl pos normal color
			char	sp[64],
					sd[64],
					sc[64];
			if (sscanf(p+3, "%63s %63s %63s", sp, sd, sc) != 3) { fclose(f); return 1; }
			Vec3	pos,
					dir,
					col; 
			
			parseVec3(sp, pos); 
			parseVec3(sd, dir);
			parseColor(sc, col);
			scene->objects.push_back(std::make_unique<Plane>(pos, dir, col));
		
			s_obj* o = new s_obj{OBJ_PLANE, pos, dir.normalized(), 0, 0, col, nullptr};
			o->next = scene->objs;
			scene->objs=o;
		} else if (strncmp(p, "cyl ", 4) == 0 || strncmp(p, "cy ", 3) == 0){ // cylindre : cyl pos dir radius height color
			// cyl pos dir radius height color  |  ex: cyl 0,-1,0 0,1,0 1 2 255,0,0
			bool	isCyShort = (strncmp(p, "cy ", 3) == 0);
			const char* rest = isCyShort ? p+3 : p+4;
			char	sp[64], sd[64], sc[64];
			double	rad, hgt;
			if (sscanf(rest, "%63s %63s %lf %lf %63s", sp, sd, &rad, &hgt, sc) != 5){
				fprintf(stderr, "Error line %d: cyl pos dir radius height color\n", no);
				fclose(f); return 1;
			}
			Vec3	pos, dir, col;
			if (!parseVec3(sp, pos) || !parseVec3(sd, dir) || !parseColor(sc, col)){
				fprintf(stderr, "Error line %d: cyl parseVec3/parseColor\n", no);
				fclose(f); return 1;
			}
			if (rad <= 0 || hgt <= 0){
				fprintf(stderr, "Error line %d: cyl radius/height >0\n", no);
				fclose(f); return 1;
			}
			scene->objects.push_back(std::make_unique<Cylinder>(pos, dir, rad, hgt, col));
			s_obj* o = new s_obj{OBJ_CYLINDER, pos, dir.normalized(), rad, hgt, col, nullptr};
			o->next = scene->objs;
			scene->objs = o;
		} else if (strncmp(p, "cone ", 5) == 0 || strncmp(p, "co ", 3) == 0){ // cône : base pos, apex = pos+dir*h
			// cone pos dir radius height color | base pos, apex = pos+dir*height, radius at base
			bool isCoShort = (strncmp(p, "co ", 3) == 0);
			const char* rest = isCoShort ? p+3 : p+5;
			char sp[64], sd[64], sc[64];
			double rad, hgt;
			if (sscanf(rest, "%63s %63s %lf %lf %63s", sp, sd, &rad, &hgt, sc) != 5){
				fprintf(stderr, "Error line %d: cone pos dir radius height color\n", no);
				fclose(f); return 1;
			}
			Vec3 pos, dir, col;
			if (!parseVec3(sp, pos) || !parseVec3(sd, dir) || !parseColor(sc, col)){
				fprintf(stderr, "Error line %d: cone parseVec3/parseColor\n", no);
				fclose(f); return 1;
			}
			if (rad <= 0 || hgt <= 0){
				fprintf(stderr, "Error line %d: cone radius/height >0\n", no);
				fclose(f); return 1;
			}
			scene->objects.push_back(std::make_unique<Cone>(pos, dir, rad, hgt, col));
			s_obj* o = new s_obj{OBJ_CONE, pos, dir.normalized(), rad, hgt, col, nullptr};
			o->next = scene->objs;
			scene->objs = o;
		} else if (strncmp(p, "parab ", 6) == 0 || strncmp(p, "paraboloid ", 11) == 0){ // paraboloïde : y = x²/a²+z²/b², 0..h
			// parab pos dir a b height color  | y = x²/a² + z²/b², tronqué 0..h (optimisé cached basis)
			bool isLong = (strncmp(p, "paraboloid ", 11) == 0);
			const char* rest = isLong ? p+11 : p+6;
			char sp[64], sd[64], sc[64];
			double a,b,hgt;
			if (sscanf(rest, "%63s %63s %lf %lf %lf %63s", sp, sd, &a, &b, &hgt, sc) != 6){
				fprintf(stderr, "Error line %d: parab pos dir a b height color\n", no);
				fclose(f); return 1;
			}
			Vec3 pos, dir, col;
			if (!parseVec3(sp, pos) || !parseVec3(sd, dir) || !parseColor(sc, col)){
				fprintf(stderr, "Error line %d: parab parse\n", no); fclose(f); return 1;
			}
			if (a <= 0 || b <= 0 || hgt <= 0){ fprintf(stderr, "Error line %d: parab a/b/h >0\n", no); fclose(f); return 1; }
			scene->objects.push_back(std::make_unique<Paraboloid>(pos, dir, a, b, hgt, col));
			s_obj* o = new s_obj{OBJ_PARABOLOID, pos, dir.normalized(), a, hgt, col, nullptr};
			o->next = scene->objs; scene->objs = o;
		} else if (strncmp(p, "hyp ", 4) == 0 || strncmp(p, "hyperboloid ", 12) == 0){ // hyperboloïde : ±1 = 1/2 nappes, ±h ou infini
			// hyp pos dir a b c sheet height color | sheet=1 (1 nappe) / 0 ou 2 (2 nappes)
			// ex: hyp 0,0,0 0,1,0 1 1 1 1 2 255,0,0  -> a=1 b=1 c=1 sheet=1 h=2
			bool isLong = (strncmp(p, "hyperboloid ", 12) == 0);
			const char* rest = isLong ? p+12 : p+4;
			char sp[64], sd[64], sc[64];
			double a,b,c,hgt;
			int sheet=1;
			int n = sscanf(rest, "%63s %63s %lf %lf %lf %d %lf %63s", sp, sd, &a, &b, &c, &sheet, &hgt, sc);
			if (n != 8){ // sans height -> version infinie
				// fallback infini sans height: hyp pos dir a b c sheet color
				n = sscanf(rest, "%63s %63s %lf %lf %lf %d %63s", sp, sd, &a, &b, &c, &sheet, sc);
				if (n != 7){ fprintf(stderr, "Error line %d: hyp pos dir a b c sheet height color (ou sans height)\n", no); fclose(f); return 1; }
				Vec3 pos, dir, col;
				if (!parseVec3(sp, pos) || !parseVec3(sd, dir) || !parseColor(sc, col)){ fclose(f); return 1; }
				if (a<=0||b<=0||c<=0){ fclose(f); return 1; }
				bool oneSheet = (sheet==1);
				scene->objects.push_back(std::make_unique<Hyperboloid>(pos, dir, a, b, c, oneSheet, col));
				s_obj* o = new s_obj{OBJ_HYPERBOLOID, pos, dir.normalized(), a, 0, col, nullptr};
				o->next = scene->objs; scene->objs = o;
			} else {
				Vec3 pos, dir, col;
				if (!parseVec3(sp, pos) || !parseVec3(sd, dir) || !parseColor(sc, col)){ fclose(f); return 1; }
				if (a<=0||b<=0||c<=0||hgt<=0){ fclose(f); return 1; }
				bool oneSheet = (sheet==1);
				scene->objects.push_back(std::make_unique<Hyperboloid>(pos, dir, a, b, c, oneSheet, hgt, col));
				s_obj* o = new s_obj{OBJ_HYPERBOLOID, pos, dir.normalized(), a, hgt, col, nullptr};
				o->next = scene->objs; scene->objs = o;
			}
		} else if (strncmp(p, "mat ", 4) == 0){ // matériau du dernier objet : mat checker scale color | diffuse
			// Phase 2 étape 1 : `mat checker <scale> <r,g,b>` appliqué au dernier objet
			// ex: mat checker 8 0,0,0  |  mat diffuse
			const char* rest = p+4;
			char kind[32] = {0};
			if (sscanf(rest, "%31s", kind) != 1){ fprintf(stderr, "Error line %d: mat <checker|diffuse>\n", no); fclose(f); return 1; }
			if (strcmp(kind, "checker") == 0){
				char sc[64];
				double scale = 8;
				if (sscanf(rest, "%*s %lf %63s", &scale, sc) != 2){ fprintf(stderr, "Error line %d: mat checker scale r,g,b\n", no); fclose(f); return 1; }
				Vec3 col2;
				if (!parseColor(sc, col2)){ fprintf(stderr, "Error line %d: mat checker color\n", no); fclose(f); return 1; }
				if (scale <= 0){ fprintf(stderr, "Error line %d: mat checker scale >0\n", no); fclose(f); return 1; }
				if (scene->objects.empty()){ fprintf(stderr, "Error line %d: mat checker sans objet avant\n", no); fclose(f); return 1; }
				auto& o = scene->objects.back();
				o->material.useChecker = true;
				o->material.uvScale = scale;
				o->material.albedo = o->color;
				o->material.albedo2 = col2;
			} else if (strcmp(kind, "diffuse") == 0){
				if (scene->objects.empty()){ fprintf(stderr, "Error line %d: mat diffuse sans objet avant\n", no); fclose(f); return 1; }
				auto& o = scene->objects.back();
				o->material.useChecker = false;
				o->material.type = MaterialType::Diffuse;
				o->material.albedo = o->color;
			} else { fprintf(stderr, "Error line %d: mat inconnu '%s'\n", no, kind); fclose(f); return 1; }
		} else if (strncmp(p, "tex ", 4) == 0){ // texture du dernier objet : tex image path [repeat|clamp] | off
			// Phase 2 : `tex image <path> [repeat|clamp]` appliqué au dernier objet
			// ex: tex image assets/brick.ppm repeat  |  tex off
			const char* rest = p+4;
			char kind[32] = {0};
			if (sscanf(rest, "%31s", kind) != 1){ fprintf(stderr, "Error line %d: tex <image|off>\n", no); fclose(f); return 1; }
			if (strcmp(kind, "off") == 0){
				if (scene->objects.empty()){ fprintf(stderr, "Error line %d: tex off sans objet avant\n", no); fclose(f); return 1; }
				auto& o = scene->objects.back();
				o->material.useImage = false;
				o->material.map.reset();
			} else if (strcmp(kind, "image") == 0){
				char spath[1024] = {0};
				char smode[32] = {0};
				// chemin + wrap optionnel (défaut repeat)
				int n = sscanf(rest, "%*s %1023s %31s", spath, smode);
				if (n < 1){ fprintf(stderr, "Error line %d: tex image <path> [repeat|clamp]\n", no); fclose(f); return 1; }
				if (scene->objects.empty()){ fprintf(stderr, "Error line %d: tex image sans objet avant\n", no); fclose(f); return 1; }
				TexWrap wrap = TexWrap::Repeat;
				if (n >= 2) {
					if (strcmp(smode, "clamp") == 0) wrap = TexWrap::Clamp;
					else if (strcmp(smode, "repeat") == 0) wrap = TexWrap::Repeat;
					else { fprintf(stderr, "Error line %d: tex wrap '%s' (repeat|clamp)\n", no, smode); fclose(f); return 1; }
				}
				auto tex = std::make_shared<ImageTexture>();
				tex->wrap = wrap;
				if (!tex->load(spath)){
					fprintf(stderr, "Error line %d: tex image introuvable '%s'\n", no, spath);
					fclose(f); return 1;
				}
				auto& o = scene->objects.back();
				o->material.map = tex;
				o->material.useImage = true;
			} else { fprintf(stderr, "Error line %d: tex inconnu '%s'\n", no, kind); fclose(f); return 1; }
		}
	}
	fclose(f); 
	return 0;
}

/* Compat C : libère via Scene::clear. */
void scene_free(Scene* s) { 
	if (s) s->clear(); 
}
