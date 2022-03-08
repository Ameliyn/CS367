#include "../FPToolkit.c"
#include "../M3d_matrix_tools.c"


double obmat[100][4][4] ;
double obinv[100][4][4] ;
double color[100][3] ;
int    num_objects ;


double partial_der_hyper(double xyz[3], int n){
  if(n == 1)
    return -1*xyz[n]*2;
  return xyz[n]*2;
}

double partial_der_circle(double xyz[3], int n){

  return xyz[n]*2;
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


int hyperbola_equat(double rayA[3], double rayB[3], double t[2]){
  double dx = (rayB[0] - rayA[0]);
  double dy = (rayB[1] - rayA[1]);
  
  double A = (dx*dx - dy*dy);
  double B = 2*rayA[0]*dx - 2*rayA[1]*dy;
  double C = rayA[0]*rayA[0] - rayA[1]*rayA[1] - 1;

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

//2d quadratic
int quadratic(double rayA[3], double rayB[3], double t[2]){
  double dx = (rayB[0] - rayA[0]);
  double dy = (rayB[1] - rayA[1]);
  
  double A = dx*dx + dy*dy;
  double B = 2*rayA[0]*dx + 2*rayA[1]*dy;
  double C = rayA[0]*rayA[0] + rayA[1]*rayA[1] - 1;

  if(B*B - 4*A*C < 0) return 0;

  if(B*B - 4*A*C == 0) {
    t[0] = (-B + sqrt(B*B - 4*A*C)) / (2*A);
    return 1;
  }
  t[0] = (-B + sqrt(B*B - 4*A*C)) / (2*A);
  t[1] = (-B - sqrt(B*B - 4*A*C)) / (2*A);
  return 2;  
}

int find_normal(int onum, double pointA[3], double xyz[3], double(*F)(double pt[3], int n)){

  double temp[3];
  M3d_mat_mult_pt(temp, obinv[onum], pointA);
  xyz[0] = obinv[onum][0][0]*F(temp,0) + obinv[onum][1][0]*F(temp,1);
  xyz[1] = obinv[onum][0][1]*F(temp,0) + obinv[onum][1][1]*F(temp,1);
  xyz[2] = 0;
  
  return 1;

  /*
  // 3d normal vector (2*temp is the partial derivative of the equation x^2 + y^2 + z^2 + 1 = 0
  xyz[0] = obinv[onum][0][0]*2*temp[0] + obinv[onum][1][0]*2*temp[1] + obinv[onum][2][0]*2*temp[2];
  xyz[1] = obinv[onum][0][1]*2*temp[0] + obinv[onum][1][1]*2*temp[1] + obinv[onum][2][1]*2*temp[2];
  xyz[2] = obinv[onum][0][2]*2*temp[0] + obinv[onum][1][2]*2*temp[1] + obinv[onum][2][2]*2*temp[2];
  */
  /*
    x^2 +xy + y^2 -1 => <2x+y, x+2y>

   */
}

int ray (double Rsource[3], double Rtip[3], double argb[3]){
  
  double t[2];
  double rayA[3];
  double rayB[3];
  int n;
  double minT = 1e50;
  int saved_onum;
  
  for(int i = 0; i < num_objects; i++){
    M3d_mat_mult_pt(rayA, obinv[i], Rsource);
    M3d_mat_mult_pt(rayB, obinv[i], Rtip);

    if(i < num_objects - 2)
      n = quadratic(rayA, rayB, t);
    else
      n = hyperbola_equat(rayA,rayB,t);
    
    if (n == 0) continue;

    for(int j = 0; j < n; j++){
      if(t[j] > 0 && t[j] < minT) {
	minT = t[j];
	saved_onum = i;
	argb[0] = color[i][0];
	argb[1] = color[i][1];
	argb[2] = color[i][2];
      }
    }
  }

  if(minT == 1e50){
    return -1;
  }

  double intersection[3], normal[3];
  intersection[0] = Rsource[0] + minT*(Rtip[0] - Rsource[0]);
  intersection[1] = Rsource[1] + minT*(Rtip[1] - Rsource[1]);
  intersection[2] = Rsource[2] + minT*(Rtip[2] - Rsource[2]);
  
  G_rgb(argb[0],argb[1],argb[2]);
  G_fill_circle(Rtip[0],Rtip[1],1);
  G_fill_circle(intersection[0],intersection[1],1);
  G_rgb(0.5,0.5,0.5);
  G_line(Rtip[0],Rtip[1],intersection[0],intersection[1]);
  if(saved_onum + 2 < num_objects)
    find_normal(saved_onum, intersection,    normal,partial_der_circle);
  else
    find_normal(saved_onum, intersection,    normal,partial_der_hyper);
  //printf("vector: %lf %lf %lf\n",normal[0], normal[1], normal[2]);
  //printf("intersection: %lf %lf %lf\n",intersection[0], intersection[1], intersection[2]);

  
  //make normal length 1
  double len ;
  double N[3] ; 
  len = sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]) ;
  if (len == 0) return 0 ;
  N[0] = normal[0]/len ;  N[1] = normal[1]/len ;  N[2] = normal[2]/len ;
  //printf("unit normal: %lf %lf %lf\n",N[0], N[1], N[2]);
  
  G_line(intersection[0] + 20*N[0], intersection[1] + 20*N[1], intersection[0], intersection[1]);
  return 1;
  
}


void Draw_ellipsoid (int onum)
{
  int n,i ;
  double t, xyz[3] ;
  double x,y ;

  G_rgb (color[onum][0],color[onum][1],color[onum][2]) ;
  
  n = 1000 ;
  for (i = 0 ; i < n ; i++) {
    t = i*2*M_PI/n ;
    xyz[0] = cos(t) ;
    xyz[1] = sin(t) ;
    xyz[2] = 0 ;
    M3d_mat_mult_pt(xyz, obmat[onum], xyz) ;
    x = xyz[0] ;
    y = xyz[1] ;
    G_point(x,y) ;
  }

}

void Draw_hyperbola (int onum)
{
  int n,i ;
  double t, xyz[3] ;
  double x,y ;

  G_rgb (color[onum][0],color[onum][1],color[onum][2]) ;
  
  n = 10000 ;
  for (i = 0 ; i < n ; i++) {
    t = i*2*M_PI/n ;
    if(cos(t) == 0) continue;
    xyz[0] = 1/cos(t) ;
    xyz[1] = tan(t) ;
    if(xyz[1] > 2 || xyz[1] < -2) continue;
    xyz[2] = 0 ;
    M3d_mat_mult_pt(xyz, obmat[onum], xyz) ;
    x = xyz[0] ;
    y = xyz[1] ;
    G_point(x,y) ;
  }

}


void Draw_the_scene()
{
  int onum ;
  for (onum = 0 ; onum < num_objects ; onum++) {
    if(onum > num_objects - 3)
      Draw_hyperbola(onum);
    else Draw_ellipsoid(onum) ;
  }
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////





int test01()
{
  double vm[4][4], vi[4][4];
  double Tvlist[100];
  int Tn, Ttypelist[100];
  double m[4][4], mi[4][4];
  double Rsource[3];
  double Rtip[3];
  double argb[3] ;

    //////////////////////////////////////////////////////////////////////
    M3d_make_identity(vm) ;    M3d_make_identity(vi) ; // OVERRIDE for 2d
    //////////////////////////////////////////////////////////////////////

    num_objects = 0 ;

    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    color[num_objects][0] = 0.0 ;
    color[num_objects][1] = 0.8 ; 
    color[num_objects][2] = 0.0 ;
	
    Tn = 0 ;
    Ttypelist[Tn] = SX ; Tvlist[Tn] =   60   ; Tn++ ;
    Ttypelist[Tn] = SY ; Tvlist[Tn] =  100   ; Tn++ ;
    Ttypelist[Tn] = RZ ; Tvlist[Tn] =   25   ; Tn++ ;
    Ttypelist[Tn] = TX ; Tvlist[Tn] =  300   ; Tn++ ;
    Ttypelist[Tn] = TY ; Tvlist[Tn] =  200   ; Tn++ ;
	
    M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
    M3d_mat_mult(obmat[num_objects], vm, m) ;
    M3d_mat_mult(obinv[num_objects], mi, vi) ;

    num_objects++ ; // don't forget to do this

    //////////////////////////////////////////////////////////////
    color[num_objects][0] = 1.0 ;
    color[num_objects][1] = 0.3 ; 
    color[num_objects][2] = 0.0 ;
	
    Tn = 0 ;
    Ttypelist[Tn] = SX ; Tvlist[Tn] =  180   ; Tn++ ;
    Ttypelist[Tn] = SY ; Tvlist[Tn] =   40   ; Tn++ ;
    Ttypelist[Tn] = RZ ; Tvlist[Tn] =   60   ; Tn++ ;
    Ttypelist[Tn] = TX ; Tvlist[Tn] =  400   ; Tn++ ;
    Ttypelist[Tn] = TY ; Tvlist[Tn] =  550   ; Tn++ ;
	
    M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
    M3d_mat_mult(obmat[num_objects], vm, m) ;
    M3d_mat_mult(obinv[num_objects], mi, vi) ;

    num_objects++ ; // don't forget to do this
    
    //////////////////////////////////////////////////////////////
    color[num_objects][0] = 0.3 ;
    color[num_objects][1] = 0.3 ; 
    color[num_objects][2] = 1.0 ;
	
    Tn = 0 ;
    Ttypelist[Tn] = SX ; Tvlist[Tn] =  175   ; Tn++ ;
    Ttypelist[Tn] = SY ; Tvlist[Tn] =   35   ; Tn++ ;
    Ttypelist[Tn] = RZ ; Tvlist[Tn] =  150   ; Tn++ ;
    Ttypelist[Tn] = TX ; Tvlist[Tn] =  360   ; Tn++ ;
    Ttypelist[Tn] = TY ; Tvlist[Tn] =  500   ; Tn++ ;
	
    M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
    M3d_mat_mult(obmat[num_objects], vm, m) ;
    M3d_mat_mult(obinv[num_objects], mi, vi) ;

    num_objects++ ; // don't forget to do this
    
    //////////////////////////////////////////////////////////////
    color[num_objects][0] = 0.5 ;
    color[num_objects][1] = 1.0 ; 
    color[num_objects][2] = 1.0 ;
	
    Tn = 0 ;
    Ttypelist[Tn] = SX ; Tvlist[Tn] =  130   ; Tn++ ;
    Ttypelist[Tn] = SY ; Tvlist[Tn] =   30   ; Tn++ ;
    Ttypelist[Tn] = RZ ; Tvlist[Tn] =  -15   ; Tn++ ;
    Ttypelist[Tn] = TX ; Tvlist[Tn] =  100   ; Tn++ ;
    Ttypelist[Tn] = TY ; Tvlist[Tn] =  700   ; Tn++ ;
	
    M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
    M3d_mat_mult(obmat[num_objects], vm, m) ;
    M3d_mat_mult(obinv[num_objects], mi, vi) ;

    num_objects++ ; // don't forget to do this
    
    //////////////////////////////////////////////////////////////
    color[num_objects][0] = 0.5 ;
    color[num_objects][1] = 1.0 ; 
    color[num_objects][2] = 0.5 ;
	
    Tn = 0 ;
    Ttypelist[Tn] = SX ; Tvlist[Tn] =   30   ; Tn++ ;
    Ttypelist[Tn] = SY ; Tvlist[Tn] =   15   ; Tn++ ;
    Ttypelist[Tn] = RZ ; Tvlist[Tn] =  -15   ; Tn++ ;
    Ttypelist[Tn] = TX ; Tvlist[Tn] =  350   ; Tn++ ;
    Ttypelist[Tn] = TY ; Tvlist[Tn] =  300   ; Tn++ ;
	
    M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
    M3d_mat_mult(obmat[num_objects], vm, m) ;
    M3d_mat_mult(obinv[num_objects], mi, vi) ;

    num_objects++ ; // don't forget to do this

    //////////////////////////////////////////////////////////////
    color[num_objects][0] = 0.5 ;
    color[num_objects][1] = 0.5 ; 
    color[num_objects][2] = 1.0 ;
	
    Tn = 0 ;
    Ttypelist[Tn] = SX ; Tvlist[Tn] =   40   ; Tn++ ;
    Ttypelist[Tn] = SY ; Tvlist[Tn] =  -50   ; Tn++ ;
    Ttypelist[Tn] = RZ ; Tvlist[Tn] =   40   ; Tn++ ;
    Ttypelist[Tn] = TX ; Tvlist[Tn] =  575   ; Tn++ ;
    Ttypelist[Tn] = TY ; Tvlist[Tn] =  350   ; Tn++ ;
	
    M3d_make_movement_sequence_matrix(m, mi, Tn, Ttypelist, Tvlist);
    M3d_mat_mult(obmat[num_objects], vm, m) ;
    M3d_mat_mult(obinv[num_objects], mi, vi) ;

    num_objects++ ; // don't forget to do this

    //////////////////////////////////////////////////////////////

    G_rgb(0,0,0) ;
    G_clear() ;

    Draw_the_scene() ;
    
    Rsource[0] =  20 ;  Rsource[1] =  400 ;  Rsource[2] = 0 ;    
    G_rgb(1,0,1) ; G_fill_circle(Rsource[0], Rsource[1], 3) ;
    G_rgb(1,0,1) ; G_line(100,200,  100,600) ;
    
    G_wait_key() ;
    
    double ytip ;
    for (ytip = 200 ; ytip <= 600 ; ytip++) {
      Rtip[0]    = 100 ;  Rtip[1]    = ytip ;  Rtip[2]   = 0  ;    

      G_rgb(1,1,0) ; G_line(Rsource[0],Rsource[1],  Rtip[0],Rtip[1]) ;
      ray (Rsource, Rtip, argb) ; 

      Draw_the_scene() ;
      G_wait_key() ;
    }

    G_rgb(1,1,1) ; G_draw_string("'q' to quit", 50,50) ;
    while (G_wait_key() != 'q') ;
    G_save_image_to_file("2d_Simple_Raytracer.xwd") ;
}




//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////




int main()
{
  G_init_graphics(800,800);
  test01() ;
}
