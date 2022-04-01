#include "../FPToolkit.c"
#include "../M3d_matrix_tools.c"
#include "../xwd_tools_03.c"

double eyeMat[4][4];
double eyeMatInv[4][4];
double eye[3];
double coi[3];
double up[3];
double hither = 1;
double yon = 1e50;
double obmat[100][4][4] ;
double obinv[100][4][4] ;
int obtype[100]; //0=sphere, 1=plane, 2=hyperbaloid, 3=cylinder
double color[100][3] ;
double objreflectivity[100]; //[0,1] percent reflectivity, -1 for no light/reflection model
char *objtexture[100];
int objtexmap[100];
int    num_objects ;
int reflection_limit = 6 ;
int scrnsize = 800;
double worldrgb[3] = {0.2,0.2,0.2};

double sphere_radius = 10;
int earthrotate = 0;
int save_files = 0;
int display_image = 1;
int fileCounter = 0;
int fileLimit = 119;
char *file_prefix = "raytext";
char *file_suffix = ".xwd";
char *directory = "texturemovie/";

//Support Light model
double light_in_world_space[3] = {0,20,30};
double light_in_eye_space[3];
double AMBIENT      = 0.2 ;
double MAX_DIFFUSE  = 0.5 ;
double SPECPOW      = 50 ;

//Object 0
double sphere_deriv(double xyz[3], int n){
  return xyz[n]*2;
}

//Object 1
double plane_deriv(double xyz[3], int n){
  if (n == 2)
    return 1;
  return 0;
}

//Object 2
double hyperbola_deriv(double xyz[3], int n){
  if(n == 1)
    return -1*xyz[n]*2;
  return xyz[n]*2;
}

//Object 3
double cylinder_deriv(double xyz[3], int n){
  if (n == 1) return 0;
  return 2*xyz[n];
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

double sphere_intercept(double rayA[3], double rayB[3], double t[2]){

  double dx = rayB[0] - rayA[0];
  double dy = rayB[1] - rayA[1];
  double dz = rayB[2] - rayA[2];

  double A = dx*dx + dy*dy + dz*dz;
  double B = 2*rayA[0]*dx + 2*rayA[1]*dy + 2*rayA[2]*dz;
  double C = rayA[0]*rayA[0] + rayA[1]*rayA[1] + rayA[2]*rayA[2] - 1;
  
  if(B*B - 4*A*C < 0) return 0;
  if(B*B - 4*A*C == 0) {
    t[0] = (-B + sqrt(B*B - 4*A*C)) / (2*A);
    return 1;
  }
  t[0] = (-B + sqrt(B*B - 4*A*C)) / (2*A);
  t[1] = (-B - sqrt(B*B - 4*A*C)) / (2*A);
  return 2; 

}

//unit plane on y axis from x [-1,1] y [-1,1]
double plane_intercept(double rayA[3], double rayB[3], double t[2]){

  double dx = rayB[0] - rayA[0];
  double dy = rayB[1] - rayA[1];
  double dz = rayB[2] - rayA[2];

  t[1] = -1;
  if(dz == 0) {
    if(rayA[0] == 0 && dx != 0) {
      t[0] = (1 - rayA[0]) / dx;
      return 1;
    }
    return 0;
  }

  t[0] = -1*rayA[2] / dz;
  if(rayA[0] + t[0]*dx < -1 || rayA[0] + t[0]*dx > 1 ||
     rayA[1] + t[0]*dy < -1 || rayA[1] + t[0]*dy > 1)
    return 0;
  return 1;
}

double hyperbola_intercept(double rayA[3], double rayB[3], double t[2]){

  double dx = rayB[0] - rayA[0];
  double dy = rayB[1] - rayA[1];
  double dz = rayB[2] - rayA[2];

  double A = dx*dx - dy*dy + dz*dz;
  double B = 2*rayA[0]*dx - 2*rayA[1]*dy + 2*rayA[2]*dz;
  double C = rayA[0]*rayA[0] - rayA[1]*rayA[1] + rayA[2]*rayA[2] - 1;

  if(B*B - 4*A*C < 0) return 0;

  if(B*B - 4*A*C == 0) {
    t[0] = (-B + sqrt(B*B - 4*A*C)) / (2*A);
    if(rayA[1] + t[0]*dy > 1 || rayA[1] + t[0]*dy < -1) return 0;
    return 1;
  }
  t[0] = (-B + sqrt(B*B - 4*A*C)) / (2*A);
  if(rayA[1] + t[0]*dy > 1 || rayA[1] + t[0]*dy < -1){
    t[0] = (-B - sqrt(B*B - 4*A*C)) / (2*A);
    if(rayA[1] + t[0]*dy > 1 || rayA[1] + t[0]*dy < -1)
      return 0;
    return 1;
  }
  t[1] = (-B - sqrt(B*B - 4*A*C)) / (2*A);
  if(rayA[1] + t[1]*dy > 1 || rayA[1] + t[1]*dy < -1)
      return 1;
  return 2; 

}

double cylinder_intercept(double rayA[3], double rayB[3], double t[2]){

  double dx = rayB[0] - rayA[0];
  double dy = rayB[1] - rayA[1];
  double dz = rayB[2] - rayA[2];

  double A = dx*dx  + dz*dz;
  double B = 2*rayA[0]*dx + 2*rayA[2]*dz;
  double C = rayA[0]*rayA[0] + rayA[2]*rayA[2] - 1;

  if(B*B - 4*A*C < 0) return 0;

  if(B*B - 4*A*C == 0) {
    t[0] = (-B + sqrt(B*B - 4*A*C)) / (2*A);
    if(rayA[1] + t[0]*dy > 1 || rayA[1] + t[0]*dy < -1) return 0;
    return 1;
  }
  t[0] = (-B + sqrt(B*B - 4*A*C)) / (2*A);
  if(rayA[1] + t[0]*dy > 1 || rayA[1] + t[0]*dy < -1){
    t[0] = (-B - sqrt(B*B - 4*A*C)) / (2*A);
    if(rayA[1] + t[0]*dy > 1 || rayA[1] + t[0]*dy < -1)
      return 0;
    return 1;
  }
  t[1] = (-B - sqrt(B*B - 4*A*C)) / (2*A);
  if(rayA[1] + t[1]*dy > 1 || rayA[1] + t[1]*dy < -1)
      return 1;
  return 2; 

}

//unit circle plane on y axis from x [-1,1] y [-1,1]
double circle_plane_intercept(double rayA[3], double rayB[3], double t[2]){

  double dx = rayB[0] - rayA[0];
  double dy = rayB[1] - rayA[1];
  double dz = rayB[2] - rayA[2];

  t[1] = -1;
  if(dz == 0) {
    if(rayA[0] == 0 && dx != 0) {
      t[0] = (1 - rayA[0]) / dx;
      return 1;
    }
    return 0;
  }

  t[0] = -1*rayA[2] / dz;
  double x = rayA[0] + t[0]*dx;
  double y = rayA[1] + t[0]*dy;
  double x2y2 = x*x + y*y;
  if(x2y2 > 1)
    return 0;
  return 1;
}

//handle which function to use
int object_intercept(double rayA[3], double rayB[3], double t[2], int onum){

  if(obtype[onum] == 0)
    return sphere_intercept(rayA, rayB, t);
  else if(obtype[onum] == 1)
    return plane_intercept(rayA,rayB,t);
  else if(obtype[onum] == 2)
    return hyperbola_intercept(rayA,rayB,t);
  else if(obtype[onum] == 3)
    return cylinder_intercept(rayA,rayB,t);
  else if(obtype[onum] == 4)
    return circle_plane_intercept(rayA,rayB,t);
  
  printf("OBJECT TYPE NOT FOUND FOR OBJECT %d... DEFAULTING TO SPHERE\n", onum);
  return sphere_intercept(rayA, rayB, t);

}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

//x = sqrt(1-v*v) * cos(u)
//y = v
//z = sqrt(1-v*v) * sin(u)
int sphere_point_to_parametric(double uvrat[2], double intersect[3], int onum){
  double u,v;
  double ulo = -M_PI ;  double uhi = M_PI;
  double vlo = -1;  double vhi = 1;

  v = intersect[1];
  if(v == 1 || v == -1){
    printf("TOP OF SPHERE\n");
    u = 0;}
  else{
    u = atan2(intersect[0] / sqrt(1-v*v), intersect[2] / sqrt(1-v*v));}

  uvrat[0] = (u - ulo) / (uhi-ulo);
  uvrat[1] = (v - vlo) / (vhi-vlo);
  return 1;
}

int plane_point_to_parametric(double uvrat[2], double intersect[3], int onum){
  double u,v;
  double ulo = -1 ;  double uhi = 1;
  double vlo = -1 ;  double vhi = 1;

  u = intersect[0];
  v = intersect[1];

  uvrat[0] = (u - ulo) / (uhi-ulo);
  uvrat[1] = (v - vlo) / (vhi-vlo);

  return 1;
}

int hyperbola_point_to_parametric(double uvrat[2], double intersect[3], int onum){
  double u,v;
  double ulo = -M_PI ;  double uhi = M_PI;
  double vlo = -1;  double vhi = 1;

  v = intersect[1];
  if(v == 1 || v == -1){
    printf("TOP OF HYPERBOLA\n");
    u = 0;}
  else{
    u = atan2(intersect[0] / sqrt(1+v*v), intersect[2] / sqrt(1+v*v));}


  uvrat[0] = (u - ulo) / (uhi-ulo);
  uvrat[1] = (v - vlo) / (vhi-vlo);

  return 1;
}

int cylinder_point_to_parametric(double uvrat[2], double intersect[3], int onum){
  double u,v;
  double ulo = -M_PI ;  double uhi = M_PI;
  double vlo = -1;  double vhi = 1;
  v = intersect[1];
  u = atan2(intersect[2],intersect[0]);

  uvrat[0] = (u - ulo) / (uhi-ulo);
  uvrat[1] = (v - vlo) / (vhi-vlo);
  return 1;
}

int obj_point_to_parametric(double uvrat[2], double intersect[3], int onum){

  int n;
  if(obtype[onum] == 0)
    n = sphere_point_to_parametric(uvrat, intersect, onum);
  else if(obtype[onum] == 1)
    n = plane_point_to_parametric(uvrat, intersect, onum);
  else if(obtype[onum] == 2)
    n = hyperbola_point_to_parametric(uvrat, intersect, onum);
  else if(obtype[onum] == 3)
    n = cylinder_point_to_parametric(uvrat, intersect, onum);
  else if(obtype[onum] == 4)
    n = plane_point_to_parametric(uvrat, intersect, onum);
  return n;
}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

int normalize(double in[3], double res[3]){

  double len ;
  len = sqrt(in[0]*in[0] + in[1]*in[1] + in[2]*in[2]) ;
  if (len == 0) return 0 ;
  res[0] = in[0]/len ;  res[1] = in[1]/len ;  res[2] = in[2]/len ;
  return 1;
}

int find_normal(int onum, double intersection[3], double Rsource[3], double res[3])

// onum = object number
// intersection = intersection point
// Rsource = ray source (probably the eye)
// res = normal vector (filled by function)
// F = partial derivative function
{
  //decide which function to use
  double (*F)(double pt[3], int n);
  if (obtype[onum] == 0)
    F = sphere_deriv;
  else if(obtype[onum] == 1)
    F = plane_deriv;
  else if(obtype[onum] == 2)
    F = hyperbola_deriv;
  else if(obtype[onum] == 3)
    F = cylinder_deriv;
  else if(obtype[onum] == 4)
    F = plane_deriv;
    
  double temp[3];
  M3d_mat_mult_pt(temp, obinv[onum], intersection);
  res[0] = obinv[onum][0][0]*F(temp, 0) + obinv[onum][1][0]*F(temp, 1) + obinv[onum][2][0]*F(temp, 2);
  res[1] = obinv[onum][0][1]*F(temp, 0) + obinv[onum][1][1]*F(temp, 1) + obinv[onum][2][1]*F(temp, 2);
  res[2] = obinv[onum][0][2]*F(temp, 0) + obinv[onum][1][2]*F(temp, 1) + obinv[onum][2][2]*F(temp, 2);

  normalize(res,res);
  
  double E[3] ;
  E[0] = Rsource[0] - intersection[0] ; 
  E[1] = Rsource[1] - intersection[1] ; 
  E[2] = Rsource[2] - intersection[2] ; 
  normalize(E,E);
  double NdotE = res[0]*E[0] + res[1]*E[1] + res[2]*E[2] ;

  if(NdotE < 0){
    res[0] *= (-1.0) ;    res[1] *= (-1.0) ;    res[2] *= (-1.0) ; 
  }

  return 1;  
}

double vec_dot(double A[3], double B[3]){
  return A[0]*B[0] + A[1]*B[1] + A[2] * B[2];
}

int find_reflection(double Rtip[3], double intersection[3], double normal[3], double res[3]){

  double T[4][4], tmp[4][4];
  double reflect[4][4] = {
			  {1 - 2*normal[0]*normal[0],   -2*normal[0]*normal[1],    -2*normal[0]*normal[2], 0},
			  {-2*normal[0]*normal[1],   1 - 2*normal[1]*normal[1],    -2*normal[1]*normal[2], 0},
			  {-2*normal[0]*normal[2],      -2*normal[1]*normal[2], 1 - 2*normal[2]*normal[2], 0},
			  {0, 0, 0, 0}
  };

  M3d_make_translation(T, -intersection[0], -intersection[1], -intersection[2]);
  M3d_make_scaling(tmp, -1, -1, -1);
  M3d_mat_mult(T, tmp, T);
  M3d_mat_mult(T, reflect, T);
  M3d_make_translation(tmp, intersection[0], intersection[1], intersection[2]);
  M3d_mat_mult(T, tmp, T);
    
  M3d_mat_mult_pt(res, T, Rtip);
  normalize(res,res);
  
  return 1;
  
}

int find_intersection(double Rsource[3], double Rtip[3], double intersection[3], double normal[3]){

  double t[2];
  double rayA[3];
  double rayB[3];
  int n;
  double minT = 1e50;
  int saved_onum;
  for(int i = 0; i < num_objects; i++){
    M3d_mat_mult_pt(rayA, obinv[i], Rsource);
    M3d_mat_mult_pt(rayB, obinv[i], Rtip);

    n = object_intercept(rayA, rayB, t, i);
    
    if (n == 0) {continue; }
    for(int j = 0; j < n; j++){
      if(t[j] > 0 && t[j] < minT) {
	minT = t[j];
	saved_onum = i;
      }
    }
  }

  if(minT == 1e50){
    return -1;
  }

  
  intersection[0] = Rsource[0] + minT*(Rtip[0] - Rsource[0]);
  intersection[1] = Rsource[1] + minT*(Rtip[1] - Rsource[1]);
  intersection[2] = Rsource[2] + minT*(Rtip[2] - Rsource[2]);

  find_normal(saved_onum,intersection,Rsource,  normal);


  return saved_onum;
}

int Light_Model (double irgb[3],
                 double s[3],
                 double p[3],
                 double n[3],
                 double argb[3],
		 int onum,
		 int reflective)
// s,p,n in eyespace

// irgb == inherent color of object (input to this function)
// s = location of start of ray (probably the eye)
// p = point on object (input to this function)
// n = normal to the object at p (input to this function)
// argb == actual color of object (output of this function)
// onum = object number we're on
// globals : AMBIENT, MAX_DIFFUSE, SPECPOW, light_in_eye_space[3]

// return 1 if successful, 0 if error
{

  //handle shadows
  double LO[3];
  LO[0] = light_in_eye_space[0] - p[0] ; 
  LO[1] = light_in_eye_space[1] - p[1] ; 
  LO[2] = light_in_eye_space[2] - p[2] ;
  normalize(LO,LO);
  double intersection[3];
  double normal[3];
  int temp = find_intersection(light_in_eye_space, p, intersection, normal);
  if(temp != onum){
    double f = AMBIENT / (AMBIENT+MAX_DIFFUSE);
    argb[0] = f * irgb[0] ;
    argb[1] = f * irgb[1] ;
    argb[2] = f * irgb[2] ;

  }
  
  double len ;
  double N[3] ; 
  len = sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]) ;
  if (len == 0) return 0 ;
  N[0] = n[0]/len ;  N[1] = n[1]/len ;  N[2] = n[2]/len ;

  double E[3] ;
  E[0] = s[0] - p[0] ; 
  E[1] = s[1] - p[1] ; 
  E[2] = s[2] - p[2] ; 
  len = sqrt(E[0]*E[0] + E[1]*E[1] + E[2]*E[2]) ;
  if (len == 0) return 0 ;
  E[0] /= len ;  E[1] /= len ;  E[2] /= len ;
  double NdotE = N[0]*E[0] + N[1]*E[1] + N[2]*E[2] ;

  double L[3] ;
  L[0] = light_in_eye_space[0] - p[0] ; 
  L[1] = light_in_eye_space[1] - p[1] ; 
  L[2] = light_in_eye_space[2] - p[2] ; 
  len = sqrt(L[0]*L[0] + L[1]*L[1] + L[2]*L[2]) ;
  if (len == 0) return 0 ;
  L[0] /= len ;  L[1] /= len ;  L[2] /= len ;
  double NdotL = N[0]*L[0] + N[1]*L[1] + N[2]*L[2] ;

  double max_ambient_and_diffuse = AMBIENT + MAX_DIFFUSE ;
     // this needs to occur BEFORE you possibly jump to LLL below

  double intensity ;
  if (NdotL*NdotE < 0) {
    // eye and light are on opposite sides of polygon
    intensity = AMBIENT ; 
    goto LLL ;
  } else if ((NdotL < 0) && (NdotE < 0)) {
    // eye and light on same side but normal pointing "wrong" way
    N[0] *= (-1.0) ;    N[1] *= (-1.0) ;    N[2] *= (-1.0) ; 
    NdotL *= (-1.0) ;
    NdotE *= (-1.0) ;   // don't use NdotE below, probably should eliminate this
  }

  // ignore Blinn's variant
  double R[3] ; // Reflection vector of incoming light
  R[0] = 2*NdotL*N[0] - L[0] ;
  R[1] = 2*NdotL*N[1] - L[1] ;
  R[2] = 2*NdotL*N[2] - L[2] ;

  double EdotR = E[0]*R[0] + E[1]*R[1] + E[2]*R[2] ;

  double diffuse ;
  if (NdotL <= 0.0) { diffuse = 0.0 ; }
  else { diffuse = MAX_DIFFUSE*NdotL ; }

  double specular ;
  if (EdotR <= 0.0) { specular = 0.0 ; }
  else { specular = (1.0 - max_ambient_and_diffuse)*pow(EdotR,SPECPOW) ;}

  // printf("%lf %lf\n",diffuse,specular) ;
  intensity = AMBIENT + diffuse + specular ;
  
 LLL : ;

  double f,g ;
  if (intensity <= max_ambient_and_diffuse) {
    f = intensity / max_ambient_and_diffuse ;
    argb[0] = f * irgb[0] ;
    argb[1] = f * irgb[1] ;
    argb[2] = f * irgb[2] ;
  } else {
    f = (intensity - max_ambient_and_diffuse) / 
                           (1.0 - max_ambient_and_diffuse) ;
    g = 1.0 - f ;
    argb[0] = g * irgb[0] + f ;
    argb[1] = g * irgb[1] + f ;
    argb[2] = g * irgb[2] + f ;
  }

  return 1 ;
}

int decide_color(int saved_onum, double Rsource[3], double normal[3],
		 double intersection[3], double argb[3], int reflection_count){
  int c;
  double irgb[3], temp[3], res[3];
  if (saved_onum == -1 || reflection_count > reflection_limit) {
    return -1;
  }

  //store inherent color just in case
  double save_color[3];
  save_color[0] = color[saved_onum][0];
  save_color[1] = color[saved_onum][1];
  save_color[2] = color[saved_onum][2];

  
  if(objtexture[saved_onum] != "none"){
    //open object texture;
    int e, d[2], widthA, heightA, texx, texy;
    e = get_xwd_map_dimensions(objtexmap[saved_onum], d) ;
    if (e == -1) { printf("failure to get dimensions\n") ;  goto decideColorPostTexture; }
    widthA = d[0] ; heightA = d[1] ;
    
    //find pixel at intersection
    //translate intersection back to object space, then find texture spot
    double objspcintersect[3], uvrat[2];
    M3d_mat_mult_pt(objspcintersect, obinv[saved_onum], intersection);

    obj_point_to_parametric(uvrat, objspcintersect, saved_onum);

    texx = widthA * uvrat[0];
    texy = heightA * uvrat[1];
    //printf("texx: %d\ntexy: %d\n",texx,texy);
    
    e = get_xwd_map_color(objtexmap[saved_onum], texx,texy,color[saved_onum]) ;
    if (e == -1) {
      color[saved_onum][0] = save_color[0];
      color[saved_onum][1] = save_color[1];
      color[saved_onum][2] = save_color[2];
      printf("failure to find color object %d (type: %d)\n", saved_onum, obtype[saved_onum]) ;
      goto decideColorPostTexture; }
  }
  
 decideColorPostTexture:
  
  if(objreflectivity[saved_onum] == 0){
    
    irgb[0] = color[saved_onum][0];
    irgb[1] = color[saved_onum][1];
    irgb[2] = color[saved_onum][2];
  
    Light_Model (irgb, Rsource, intersection, normal, argb, saved_onum, 0);
  }else if(objreflectivity[saved_onum] > 0){
    //Allow for some inherent color of the mirror
    argb[0] = color[saved_onum][0] * (1-objreflectivity[saved_onum]);
    argb[1] = color[saved_onum][1] * (1-objreflectivity[saved_onum]);
    argb[2] = color[saved_onum][2] * (1-objreflectivity[saved_onum]);

    //find reflection
    find_reflection(Rsource, intersection, normal, res);

    //offset reflection vector
    temp[0] = intersection[0] + 2*res[0];
    temp[1] = intersection[1] + 2*res[1];
    temp[2] = intersection[2] + 2*res[2];
    intersection[0] += 0.1*res[0];
    intersection[1] += 0.1*res[1];
    intersection[2] += 0.1*res[2];

    //find object in mirror
    int new_onum;
    new_onum = find_intersection(intersection,temp,res, normal);
    c = decide_color(new_onum, temp, normal, res, argb, reflection_count+1);
    if(c == -1) {
      //reset color to saved color if necessary
      color[saved_onum][0] = save_color[0];
      color[saved_onum][1] = save_color[1];
      color[saved_onum][2] = save_color[2];
      return -1;}
    irgb[0] = color[saved_onum][0] * (1-objreflectivity[saved_onum]) + argb[0]*objreflectivity[saved_onum];
    irgb[1] = color[saved_onum][0] * (1-objreflectivity[saved_onum]) + argb[1]*objreflectivity[saved_onum];
    irgb[2] = color[saved_onum][0] * (1-objreflectivity[saved_onum]) + argb[2]*objreflectivity[saved_onum];
    Light_Model (irgb, temp, res, normal, argb, saved_onum, 1);

    
  }else{
    argb[0] = color[saved_onum][0];
    argb[1] = color[saved_onum][1];
    argb[2] = color[saved_onum][2];
  }

  //reset color to saved color if necessary
  color[saved_onum][0] = save_color[0];
  color[saved_onum][1] = save_color[1];
  color[saved_onum][2] = save_color[2];
  return 1;
}

int ray (double Rtip[3], double argb[3]){
  //camera_light = flag for camera or shadow (camera shows all, shadow shoots through transp)
  double Rsource[3];
  Rsource[0] = 0;
  Rsource[1] = 0;
  Rsource[2] = 0;
  double irgb[3];
  double intersection[3], normal[3], res[3], temp[3];
  argb[0] = worldrgb[0];
  argb[1] = worldrgb[1];
  argb[2] = worldrgb[2];
  int saved_onum = find_intersection(Rsource,Rtip,intersection, normal);
  decide_color(saved_onum, Rsource, normal, intersection, argb, 0);

  return 1;
  
}

int create_object_matricies(double vm[4][4], double vi[4][4]){

  double Tvlist[100];
  int Tn, Ttypelist[100];
  double m[4][4], mi[4][4];

  
  num_objects = 0 ;
    
  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////
  obtype[num_objects] = 2;
  color[num_objects][0] = 0.0 ;
  color[num_objects][1] = 0.8 ; 
  color[num_objects][2] = 0.0 ;
  objreflectivity[num_objects] = 0;
  objtexture[num_objects] = "woodgood600x300.xwd";
	
  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  10    ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  55   ; Tn++ ;
	
  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  //////////////////////////////////////////////////////////////

  obtype[num_objects] = 2;
  color[num_objects][0] = 0.0 ;
  color[num_objects][1] = 0.8 ; 
  color[num_objects][2] = 0.0 ;
  objreflectivity[num_objects] = 0;
  objtexture[num_objects] = "woodgood600x300.xwd";
	
  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  10    ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  30   ; Tn++ ;
  Ttypelist[Tn] = TX ; Tvlist[Tn] =  25   ; Tn++ ;
	
  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  //////////////////////////////////////////////////////////////
  
  //////////////////////////////////////////////////////////////

  obtype[num_objects] = 2;
  color[num_objects][0] = 0.0 ;
  color[num_objects][1] = 0.8 ; 
  color[num_objects][2] = 0.0 ;
  objreflectivity[num_objects] = 0;
  objtexture[num_objects] = "woodgood600x300.xwd";
	
  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  10    ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  30   ; Tn++ ;
  Ttypelist[Tn] = TX ; Tvlist[Tn] =  -25   ; Tn++ ;
	
  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  //////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////

  obtype[num_objects] = 2;
  color[num_objects][0] = 0.0 ;
  color[num_objects][1] = 0.8 ; 
  color[num_objects][2] = 0.0 ;
  objreflectivity[num_objects] = 0;
  objtexture[num_objects] = "woodgood600x300.xwd";
	
  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  10    ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  5    ; Tn++ ;
	
  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  //////////////////////////////////////////////////////////////

  //Chandelier
  obtype[num_objects] = 0;
  color[num_objects][0] = 0.7 ;
  color[num_objects][1] = 0.7 ; 
  color[num_objects][2] = 0.0 ;
  objreflectivity[num_objects] = 0;
  objtexture[num_objects] = "none";
	
  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  10    ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  1    ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  30    ; Tn++ ;
  Ttypelist[Tn] = TY ; Tvlist[Tn] =  25    ; Tn++ ;
	
  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  //////////////////////////////////////////////////////////////

  obtype[num_objects] = 0;
  color[num_objects][0] = 0.7 ;
  color[num_objects][1] = 0.7 ; 
  color[num_objects][2] = 0.0 ;
  objreflectivity[num_objects] = 0;
  objtexture[num_objects] = "none";
	
  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  1    ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  10    ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  30    ; Tn++ ;
  Ttypelist[Tn] = TY ; Tvlist[Tn] =  25    ; Tn++ ;
	
  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  //////////////////////////////////////////////////////////////
  
  
  //table top
  obtype[num_objects] = 4;
  color[num_objects][0] = 0.0 ;
  color[num_objects][1] = 0.4 ; 
  color[num_objects][2] = 0.4 ;
  objreflectivity[num_objects] = 0;
  objtexture[num_objects] = "graywood.xwd";
	
  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  30   ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  30   ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  30   ; Tn++ ;
  Ttypelist[Tn] = RX ; Tvlist[Tn] =  90   ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  30   ; Tn++ ;
  Ttypelist[Tn] = TY ; Tvlist[Tn] =  -10.2   ; Tn++ ;
    
  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  //////////////////////////////////////////////////////////////

  //MIRRORS
  obtype[num_objects] = 4;
  color[num_objects][0] = 0.0 ;
  color[num_objects][1] = 0.8 ; 
  color[num_objects][2] = 1.0 ;
  objreflectivity[num_objects] = 0.8;
  objtexture[num_objects] = "none";
	
  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  20   ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  20   ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  20   ; Tn++ ;
  Ttypelist[Tn] = RX ; Tvlist[Tn] =  150   ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  55   ; Tn++ ;
  Ttypelist[Tn] = TY ; Tvlist[Tn] =  20   ; Tn++ ;
    
  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  //////////////////////////////////////////////////////////////
  
  obtype[num_objects] = 1;
  color[num_objects][0] = 0.0 ;
  color[num_objects][1] = 0.8 ; 
  color[num_objects][2] = 1.0 ;
  objreflectivity[num_objects] = 0.8;
  objtexture[num_objects] = "none";

  
  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  15   ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  15   ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  15   ; Tn++ ;
  Ttypelist[Tn] = RX ; Tvlist[Tn] =  20   ; Tn++ ;
  Ttypelist[Tn] = RY ; Tvlist[Tn] =  110   ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  25   ; Tn++ ;
  Ttypelist[Tn] = TX ; Tvlist[Tn] =  -35   ; Tn++ ;
  Ttypelist[Tn] = TY ; Tvlist[Tn] =  20   ; Tn++ ;
    
  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  //////////////////////////////////////////////////////////////
  
  //world sphere
  obtype[num_objects] = 0;
  color[num_objects][0] = 0.2 ;
  color[num_objects][1] = 0.2 ; 
  color[num_objects][2] = 0.2 ;
  objreflectivity[num_objects] = -1;
  objtexture[num_objects] = "stars1024x1024.xwd";

  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  sphere_radius    ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  sphere_radius    ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  sphere_radius    ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  30     ; Tn++ ;
  Ttypelist[Tn] = TY ; Tvlist[Tn] =  20     ; Tn++ ;
  Ttypelist[Tn] = TY ; Tvlist[Tn] =  0      ; Tn++ ;
  

  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  
  //////////////////////////////////////////////////////////////
  
  //create cylinder
  obtype[num_objects] = 3;
  color[num_objects][0] = 0.8 ;
  color[num_objects][1] = 0.2 ; 
  color[num_objects][2] = 0.2 ;
  objreflectivity[num_objects] = 0;
  objtexture[num_objects] = "woodgood600x300.xwd";

	
  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  6    ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  2    ; Tn++ ;
  Ttypelist[Tn] = TY ; Tvlist[Tn] =  -5    ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  8    ; Tn++ ;
  Ttypelist[Tn] = TX ; Tvlist[Tn] =  -15    ; Tn++ ;
	
  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  //////////////////////////////////////////////////////////////

  //Floating Earth
  obtype[num_objects] = 0;
  color[num_objects][0] = 0.2 ;
  color[num_objects][1] = 0.2 ; 
  color[num_objects][2] = 0.2 ;
  objreflectivity[num_objects] = 0;
  objtexture[num_objects] = "Earthgood1024x512.xwd";
  
	
  Tn = 0 ;
  Ttypelist[Tn] = SX ; Tvlist[Tn] =  10    ; Tn++ ;
  Ttypelist[Tn] = SY ; Tvlist[Tn] =  10    ; Tn++ ;
  Ttypelist[Tn] = SZ ; Tvlist[Tn] =  10    ; Tn++ ;
  Ttypelist[Tn] = RY ; Tvlist[Tn] =  -45-earthrotate    ; Tn++ ;
  Ttypelist[Tn] = TZ ; Tvlist[Tn] =  35    ; Tn++ ;
  Ttypelist[Tn] = TY ; Tvlist[Tn] =  20    ; Tn++ ;
  Ttypelist[Tn] = TX ; Tvlist[Tn] =  50    ; Tn++ ;
	
  M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
  M3d_mat_mult(obmat[num_objects], vm, m) ;
  M3d_mat_mult(obinv[num_objects], mi, vi) ;

  num_objects++ ; // don't forget to do this
  //////////////////////////////////////////////////////////////
  
}

void Draw_the_scene()
{

  double temp[3], argb[3], vm[4][4], vi[4][4];
  temp[0] = -1;
  temp[1] = -1;
  temp[2] = hither;
  normalize(temp,temp);

  
  M3d_view(vm, vi,  eye,coi,up);
  M3d_mat_mult_pt(light_in_eye_space, vm, light_in_world_space);
  create_object_matricies(vm,vi);
  
  for(int x = 0; x < scrnsize; x++){
    for(int y = 0; y < scrnsize; y++){
      temp[0] = x / (scrnsize/2.0) - 1;
      temp[1] = y / (scrnsize/2.0) - 1;
      ray (temp, argb) ;
      G_rgb(argb[0],argb[1],argb[2]);
      G_point(x,y);
    }
  }
  if(save_files == 1){
    char fileName[100];
    sprintf(fileName,"%s%s%04d%s",directory,file_prefix,fileCounter,file_suffix);
    if (display_image == 1) fprintf(stderr,"saving image to file %s\n",fileName);
    G_save_image_to_file(fileName);
    fileCounter++;
  }
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


int openXWDfiles(){

  for(int i = 0; i < num_objects; i++){
    objtexmap[i] = init_xwd_map_from_file (objtexture[i]) ;
    if (objtexmap[i] == -1) { printf("Object %d has no texture\n",i);}

  }

}


int test03(){
  double vm[4][4], vi[4][4];
  int mode = 1;

  //////////////////////////////////////////////////////////////////////
  
  eye[0] = 0;
  eye[1] = 0;
  eye[2] = -10;
  coi[0] = 0;
  coi[1] = 0;
  coi[2] = 50;
  up[0] = 0;
  up[1] = 1;
  up[2] = 0;
  //////////////////////////////////////////////////////////////////////

  
  G_rgb(0,0,0) ;
  G_clear() ;
    
  double t = 0;
  int c;

  //handle opening the xwd files just once to prevent overflow.
  M3d_view(vm, vi,  eye,coi,up);
  create_object_matricies(vm, vi);
  openXWDfiles();
  double pi60 = M_PI/60;

  eye[0] = 25*cos(M_PI + 0);
  eye[1] = 25*sin(M_PI + 0) + 25;
    

  up[0] = eye[0];
  up[1] = eye[1] + 1;
  up[2] = eye[2];
  while(1){
    sphere_radius += 50;
    printf("sphere_radius = %02f\n",sphere_radius);

    if(mode == 1 && display_image == 1){
      Draw_the_scene() ;
      c = G_wait_key();
      if(c == 'm') mode = 0;
      if(c == 'q') break;
    }
    else if (display_image == 1){
      Draw_the_scene() ;
      G_display_image();
      c = G_no_wait_key();
      if(c == 'm') mode = 1;
      if(c == 'q') break;
    }
    else{
      Draw_the_scene();
      if(fileCounter == fileLimit) break;
    }
  }

}

int test02(){
  double vm[4][4], vi[4][4];
  int mode = 1;

  //////////////////////////////////////////////////////////////////////
  
  eye[0] = 0;
  eye[1] = 0;
  eye[2] = -10;
  coi[0] = 0;
  coi[1] = 0;
  coi[2] = 50;
  up[0] = 0;
  up[1] = 1;
  up[2] = 0;
  //////////////////////////////////////////////////////////////////////

  
  G_rgb(0,0,0) ;
  G_clear() ;
    
  double t = 0;
  int c;

  //handle opening the xwd files just once to prevent overflow.
  M3d_view(vm, vi,  eye,coi,up);
  create_object_matricies(vm, vi);
  openXWDfiles();
  double pi60 = M_PI/60;

  eye[0] = 25*cos(M_PI + 0);
  eye[1] = 25*sin(M_PI + 0) + 25;
    

  up[0] = eye[0];
  up[1] = eye[1] + 1;
  up[2] = eye[2];
  while(1){
    hither += 0.1;
    printf("hither = %02f\n",hither);

    if(mode == 1 && display_image == 1){
      Draw_the_scene() ;
      c = G_wait_key();
      if(c == 'm') mode = 0;
      if(c == 'q') break;
    }
    else if (display_image == 1){
      Draw_the_scene() ;
      G_display_image();
      c = G_no_wait_key();
      if(c == 'm') mode = 1;
      if(c == 'q') break;
    }
    else{
      Draw_the_scene();
      if(fileCounter == fileLimit) break;
    }
  }

}

int test01()
{
  double vm[4][4], vi[4][4];
  int mode = 1;

  //////////////////////////////////////////////////////////////////////
  
  eye[0] = 0;
  eye[1] = 0;
  eye[2] = -10;
  coi[0] = 0;
  coi[1] = 0;
  coi[2] = 50;
  up[0] = 0;
  up[1] = 1;
  up[2] = 0;
  //////////////////////////////////////////////////////////////////////

  
  G_rgb(0,0,0) ;
  G_clear() ;
    
  double t = 0;
  int c;

  //handle opening the xwd files just once to prevent overflow.
  M3d_view(vm, vi,  eye,coi,up);
  create_object_matricies(vm, vi);
  openXWDfiles();
  double pi60 = M_PI/60;
  while(1){
    t += pi60;
    earthrotate += 6;
    //move the eye!
    eye[0] = 25*cos(M_PI + t);
    eye[1] = 25*sin(M_PI + t) + 25;
    

    up[0] = eye[0];
    up[1] = eye[1] + 1;
    up[2] = eye[2];

    if(mode == 1 && display_image == 1){
      Draw_the_scene() ;
      c = G_wait_key();
      if(c == 'm') mode = 0;
      if(c == 'q') break;
    }
    else if (display_image == 1){
      Draw_the_scene() ;
      G_display_image();
      c = G_no_wait_key();
      if(c == 'm') mode = 1;
      if(c == 'q') break;
    }
    else{
      Draw_the_scene();
      if(fileCounter == fileLimit) break;
    }
  }
}




//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////




int main(int argc, char **argv)
{
  if(argc < 2){
    printf("Usage: ./a.out reflectionLimit\nUsing Default Reflection limit (6)\n");
  }
  else reflection_limit = atoi(argv[1]);
  G_init_graphics(scrnsize,scrnsize);
  test03() ;
}


//reflect image twice to get seamless

