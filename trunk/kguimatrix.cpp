/*********************************************************************************/
/* kGUI - kguimatrix.cpp                                                         */
/*                                                                               */
/* Initially Designed and Programmed by Kevin Pickell                            */
/*                                                                               */
/* http://code.google.com/p/kgui/                                                */
/*                                                                               */
/*    kGUI is free software; you can redistribute it and/or modify               */
/*    it under the terms of the GNU Lesser General Public License as published by*/
/*    the Free Software Foundation; version 2.                                   */
/*                                                                               */
/*    kGUI is distributed in the hope that it will be useful,                    */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/*    GNU General Public License for more details.                               */
/*                                                                               */
/*    http://www.gnu.org/licenses/lgpl.txt                                       */
/*                                                                               */
/*    You should have received a copy of the GNU General Public License          */
/*    along with kGUI; if not, write to the Free Software                        */
/*    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguimatrix.h"

void kGUIMatrix33::Identity(void)
{
	m_m[0][0]=1.0f;	m_m[0][1]=0.0f;	m_m[0][2]=0.0f;
	m_m[1][0]=0.0f;	m_m[1][1]=1.0f;	m_m[1][2]=0.0f;
	m_m[2][0]=0.0f;	m_m[2][1]=0.0f;	m_m[2][2]=1.0f;

	m_t.m_x=0.0f;
	m_t.m_y=0.0f;
	m_t.m_z=0.0f;
}

void kGUIMatrix33::SetRotX(double angle)
{
	double brad,ca,sa;
	
	brad=angle*(3.1415926535f*2.0f);
	ca=cos(brad);
	sa=sin(brad);
	
	m_m[0][0]=1.0f;	m_m[0][1]=0.0f;		m_m[0][2]=0.0f;
	m_m[1][0]=0.0f;	m_m[1][1]=ca;		m_m[1][2]=-sa;
	m_m[2][0]=0.0f;	m_m[2][1]=sa;		m_m[2][2]= ca;
}

void kGUIMatrix33::SetRotY(double angle)
{
	double brad,ca,sa;
	
	brad=angle*(3.1415926535f*2.0f);
	ca=cos(brad);
	sa=sin(brad);
	
	m_m[0][0]=ca;		m_m[0][1]=0.0f;		m_m[0][2]=sa;
	m_m[1][0]=0.0f;		m_m[1][1]=1.0f;		m_m[1][2]=0.0f;
	m_m[2][0]=-sa;		m_m[2][1]=0.0f;		m_m[2][2]=ca;
}

void kGUIMatrix33::SetRotZ(double angle)
{
	double brad,ca,sa;
	
	brad=angle*(3.1415926535f*2.0f);
	ca=cos(brad);
	sa=sin(brad);
	
	m_m[0][0]=ca;		m_m[0][1]=-sa;		m_m[0][2]=0.0f;
	m_m[1][0]=sa;		m_m[1][1]=ca;		m_m[1][2]=0.0f;
	m_m[2][0]=0.0f;		m_m[2][1]=0.0f;		m_m[2][2]=1.0f;
}

void kGUIMatrix33::SetScale(double scale)
{
	m_m[0][0]=scale;	m_m[0][1]=0.0f;		m_m[0][2]=0.0f;
	m_m[1][0]=0.0f;		m_m[1][1]=scale;	m_m[1][2]=0.0f;
	m_m[2][0]=0.0f;		m_m[2][1]=0.0f;		m_m[2][2]=scale;
}

void kGUIMatrix33::Transform(kGUIVector3 *in,kGUIVector3 *out)
{
	out->m_x = (m_m[0][0]*in->m_x+m_m[0][1]*in->m_y+m_m[0][2]*in->m_z)+m_t.m_x;
	out->m_y = (m_m[1][0]*in->m_x+m_m[1][1]*in->m_y+m_m[1][2]*in->m_z)+m_t.m_y;
	out->m_z = (m_m[2][0]*in->m_x+m_m[2][1]*in->m_y+m_m[2][2]*in->m_z)+m_t.m_z;
}

void kGUIMatrix33::Mult(kGUIMatrix33 *in,kGUIMatrix33 *out)
{
	unsigned int i,j,k;
	double r;

	for (i=0; i<3; i++)
	{
		for (j=0; j<3; j++)
		{
			r=0.0f;
			for (k=0; k<3; k++)
				r += in->m_m[i][k]*m_m[k][j];
			out->m_m[i][j]=r;
		}
	}
}

#if 0

// -------------------------------------------------------
// MATRIX 3x3 TRANSPOSE 
// carfully in case mt is the same as m
// -------------------------------------------------------
void matrix3x3_transpose(matrix3x3 mt,matrix3x3 m)
{
	int			i,j;
	float 		t;

	for (i=0; i<3; i++) {
		for (j=0; j<3; j++) {
			t = m[i][j];
			mt[i][j] = m[j][i];
			mt[j][i] = t;
		}
	}
}

// -------------------------------------------------------
// MATRIX 3x3 COFACTOR 
// -------------------------------------------------------
float matrix3x3_cofactor(matrix3x3 m,int i,int j)
{
	float	a00,a01,a10,a11;
	int		c;
	
	c = ( 2*((i+j)/2) == (i+j) ) ? 1 : -1;
	
	if (i==0) {
		if (j==0) {
			a00 = m[1][1]; a01 = m[1][2];
			a10 = m[2][1]; a11 = m[2][2];
		}
		else if (j==1) {
			a00 = m[1][0]; a01 = m[1][2];
			a10 = m[2][0]; a11 = m[2][2];
		}
		else {
			a00 = m[1][0]; a01 = m[1][1];
			a10 = m[2][0]; a11 = m[2][1];
		}
	}
	else if (i==1) {
		if (j==0) {
			a00 = m[0][1]; a01 = m[0][2];
			a10 = m[2][1]; a11 = m[2][2];
		}
		else if (j==1) {
			a00 = m[0][0]; a01 = m[0][2];
			a10 = m[2][0]; a11 = m[2][2];
		}
		else {
			a00 = m[0][0]; a01 = m[0][1];
			a10 = m[2][0]; a11 = m[2][1];
		}
	}
	else {
		if (j==0) {
			a00 = m[0][1]; a01 = m[0][2];
			a10 = m[1][1]; a11 = m[1][2];
		}
		else if (j==1) {
			a00 = m[0][0]; a01 = m[0][2];
			a10 = m[1][0]; a11 = m[1][2];
		}
		else {
			a00 = m[0][0]; a01 = m[0][1];
			a10 = m[1][0]; a11 = m[1][1];
		}
	}
	
	return (c*(a00*a11 - a01*a10));

}

// -------------------------------------------------------
// MATRIX 3x3 DETERMINANT 
// -------------------------------------------------------
float matrix3x3_determinant(matrix3x3 m)
{ 
	float	c1,c2,c3;
	
	c1 = m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1]);
	c2 = m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0]);
	c3 = m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
	// printf("%f %f %f: %f\n",c1,c2,c3,c1-c2+c3);
	return( c1 - c2 + c3);
	
}

// -------------------------------------------------------
// MATRIX 3x3 INVERSE 
// -------------------------------------------------------
void matrix3x3_inverse(matrix3x3 minv,matrix3x3 m)
{
	float		detm;
	int			i,j;

	// matrix3x3_print(m);
	detm = matrix3x3_determinant(m);
	
	// printf("determinant: %f\n",detm);
	
	for (i=0; i<3; i++) 
		for (j=0; j<3; j++)
			minv[j][i] = matrix3x3_cofactor(m,i,j)/detm;
	
}

// -------------------------------------------------------
// MATRIX 3x3 MULTIPLY: A = B.C */
// -------------------------------------------------------
void matrix3x3_multiply(matrix3x3 a,matrix3x3 b,matrix3x3 c)
{
	matrix3x3	t;
	int		i,j,k;
	
	t = matrix3x3_allocate();
	
	for (i=0; i<3; i++) {
		for (j=0; j<3; j++) {
			t[i][j] = 0.0;
			for (k=0; k<3; k++) {
				t[i][j] += b[i][k]*c[k][j];
			}
		}
	}
	for (i=0; i<3; i++)  for (j=0; j<3; j++) a[i][j] = t[i][j];
	matrix3x3_deallocate(t);
}

// -------------------------------------------------------
// MATRIX 3x3 ADD: A = B + C */
// -------------------------------------------------------
void matrix3x3_add(matrix3x3 a,matrix3x3 b,matrix3x3 c)
{
	int		i,j;
	
	for (i=0; i<3; i++) {
		for (j=0; j<3; j++) {
			a[i][j] = b[i][j]+c[i][j];
		}
	}
}

// -------------------------------------------------------
// MATRIX 3x3 SUBTRACT: A = B - C */
// -------------------------------------------------------
void matrix3x3_subtract(matrix3x3 a,matrix3x3 b,matrix3x3 c)
{
	int		i,j;
	
	for (i=0; i<3; i++) {
		for (j=0; j<3; j++) {
			a[i][j] = b[i][j]-c[i][j];
		}
	}
}


// -------------------------------------------------------
// MATRIX 3x3 MULTIPLY VECTOR: A = B.C
// -------------------------------------------------------
void matrix3x3_multiply_constant(matrix3x3 a,float c)
{
	a[0][0] *= c; a[0][1] *= c; a[0][2] *= c;
	a[1][0] *= c; a[1][1] *= c; a[1][2] *= c;
	a[2][0] *= c; a[2][1] *= c; a[2][2] *= c;
}

// -------------------------------------------------------
// MATRIX 3x3 PRINT
// -------------------------------------------------------
void matrix3x3_print(matrix3x3 a)
{
	printf("%f %f %f\n",a[0][0],a[0][1],a[0][2]);
	printf("%f %f %f\n",a[1][0],a[1][1],a[1][2]);
	printf("%f %f %f\n",a[2][0],a[2][1],a[2][2]);
}

// -------------------------------------------------------
// VECTOR 3 RIGID TRANFORM
// -------------------------------------------------------
void vector3_rigid_transform(vector3 pp,vector3 p,matrix3x3 R, vector3 T)
{
	
	matrix3x3_multiply_vector(pp,R,p);
	pp[0] += T[0];
	pp[1] += T[1];
	pp[2] += T[2];
	
}
	
// -------------------------------------------------------
// VECTOR 3 NORMALIZE
// -------------------------------------------------------
float  vector3_norm(vector3 A) 
{ 
	float	len;
	
	len = sqrt(A[0]*A[0]+A[1]*A[1]+A[2]*A[2]);
	return len;
}

// -------------------------------------------------------
// VECTOR 3 NORMALIZE
// -------------------------------------------------------
void  vector3_normalize(vector3 A) 
{ 
	float	len;
	
	len = vector3_norm(A);
	A[0] /= len;
	A[1] /= len;
	A[2] /= len;
}

// -------------------------------------------------------
// VECTOR 3 DOT PRODUCT
// -------------------------------------------------------
float  vector3_dotproduct(vector3 A, vector3 B) 
{ 
	float	len;
	
	len = A[0]*B[0]+A[1]*B[1]+A[2]*B[2];
	return len;
}

// -------------------------------------------------------
// VECTOR 3 CROSS PRODUCT
// C = A x B
// -------------------------------------------------------
void vector3_crossproduct(vector3 C,vector3 A, vector3 B) 
{ 
	C[0] = A[1]*B[2] - A[2]*B[1];
	C[1] = A[2]*B[0] - A[0]*B[2];
	C[2] = A[0]*B[1] - A[1]*B[0];
	
	return;
}

// -------------------------------------------------------
// VECTOR 3 CROSS PRODUCT
// C = A + B
// -------------------------------------------------------
void vector3_add(vector3 C,vector3 A, vector3 B) 
{ 
	C[0] = A[0]+B[0];
	C[1] = A[1]+B[1];
	C[2] = A[2]+B[2];
	
	return;
}

	
// -------------------------------------------------------
// VECTOR 3 CREATE
// -------------------------------------------------------
float *vector3_create() 
{ 
	float	*C;
	
	C = (float *)malloc(sizeof(float)*3);
	
	return C;
}

	
// -------------------------------------------------------
// VECTOR 3 AXIS ANGLE ROTATE
// angle is in degrees
// pp = p rotated around a by angle degrees
// -------------------------------------------------------
void  vector3_axisangle_rotate(vector3 pp,vector3 p,vector3 a,float angle) 
{
	matrix3x3	Abar,Astar;
	matrix3x3	identity;
	matrix3x3	m;
	float		ca,sa;
	float		d;
	double		radians;
	
	if (angle>360) { d = (int)(angle/360); angle = angle - d*360; }
	radians = angle*3.1415926535/180;	
	
	ca = cos(radians); sa = sin(radians);
	Abar = matrix3x3_allocate();
	Astar = matrix3x3_allocate();
	identity = matrix3x3_allocate();
	matrix3x3_set_identity(identity);
	m = matrix3x3_allocate();
	
	Abar[0][0] = a[0]*a[0]; Abar[0][1] = a[0]*a[1]; Abar[0][2] = a[0]*a[2];
	Abar[1][0] = a[1]*a[0]; Abar[1][1] = a[1]*a[1]; Abar[1][2] = a[1]*a[2];
	Abar[2][0] = a[2]*a[0]; Abar[2][1] = a[2]*a[1]; Abar[2][2] = a[2]*a[2];
	
	Astar[0][0] =   0.0; Astar[0][1] = -a[2]; Astar[0][2] =  a[1];
	Astar[1][0] =  a[2]; Astar[1][1] =   0.0; Astar[1][2] = -a[0];
	Astar[2][0] = -a[1]; Astar[2][1] =  a[0]; Astar[2][2] =   0.0;
	
	// M = ABar + cos*(I-Abar) + sin*Astar
	matrix3x3_subtract(m,identity,Abar);
	matrix3x3_multiply_constant(m,ca);
	matrix3x3_multiply_constant(Astar,sa);
	matrix3x3_add(m,Abar,m);
	matrix3x3_add(m,Astar,m);
	
	// pp = M P
	matrix3x3_multiply_vector(pp,m,p);
	
	matrix3x3_deallocate(identity);
	matrix3x3_deallocate(m);
}

// -------------------------------------------------------
// VECTOR 3 AXIS ANGLE ROTATE
// angle is in radians
// pp = p rotated around a by angle radians
// -------------------------------------------------------
void  vector3_axisradianangle_rotate(vector3 pp,vector3 p,vector3 a,float angle) 
{
	matrix3x3	Abar,Astar;
	matrix3x3	identity;
	matrix3x3	m;
	float		ca,sa;
	float		d;
	double		radians;
	
	if (angle>3.1415926535) { d = (int)(angle/3.1415926535); angle = angle - d*3.1415926535; }
	radians = angle;	
	
	ca = cos(radians); sa = sin(radians);
	Abar = matrix3x3_allocate();
	Astar = matrix3x3_allocate();
	identity = matrix3x3_allocate();
	matrix3x3_set_identity(identity);
	m = matrix3x3_allocate();
	
	Abar[0][0] = a[0]*a[0]; Abar[0][1] = a[0]*a[1]; Abar[0][2] = a[0]*a[2];
	Abar[1][0] = a[1]*a[0]; Abar[1][1] = a[1]*a[1]; Abar[1][2] = a[1]*a[2];
	Abar[2][0] = a[2]*a[0]; Abar[2][1] = a[2]*a[1]; Abar[2][2] = a[2]*a[2];
	
	Astar[0][0] =   0.0; Astar[0][1] = -a[2]; Astar[0][2] =  a[1];
	Astar[1][0] =  a[2]; Astar[1][1] =   0.0; Astar[1][2] = -a[0];
	Astar[2][0] = -a[1]; Astar[2][1] =  a[0]; Astar[2][2] =   0.0;
	
	// M = ABar + cos*(I-Abar) + sin*Astar
	matrix3x3_subtract(m,identity,Abar);
	matrix3x3_multiply_constant(m,ca);
	matrix3x3_multiply_constant(Astar,sa);
	matrix3x3_add(m,Abar,m);
	matrix3x3_add(m,Astar,m);
	
	// pp = M P
	matrix3x3_multiply_vector(pp,m,p);
	
	matrix3x3_deallocate(identity);
	matrix3x3_deallocate(m);
}

// ================================================================================
// 4x4 matrix routines
// ================================================================================
	
// -----------------------------------------------------------
// MATRIX SET IDENTITY 
// -----------------------------------------------------------
void matrix_set_identity(matrix m)
{
	m[ 0] = 1.0; m[ 4] = 0.0; m[ 8] = 0.0; m[12] = 0.0;
	m[ 1] = 0.0; m[ 5] = 1.0; m[ 9] = 0.0; m[13] = 0.0;
	m[ 2] = 0.0; m[ 6] = 0.0; m[10] = 1.0; m[14] = 0.0;
	m[ 3] = 0.0; m[ 7] = 0.0; m[11] = 0.0; m[15] = 1.0;
}

// -----------------------------------------------------------
/* MATRIX SET SCALE */
// -----------------------------------------------------------
void matrix_set_scale(matrix m,float sx,float sy,float sz)
{
	m[ 0] =  sx; m[ 4] = 0.0; m[ 8] = 0.0; m[12] = 0.0;
	m[ 1] = 0.0; m[ 5] =  sy; m[ 9] = 0.0; m[13] = 0.0;
	m[ 2] = 0.0; m[ 6] = 0.0; m[10] =  sz; m[14] = 0.0;
	m[ 3] = 0.0; m[ 7] = 0.0; m[11] = 0.0; m[15] = 1.0;
}

// -----------------------------------------------------------
/* MATRIX SET TRANSLATE */
// -----------------------------------------------------------
void matrix_set_translate(matrix m,float tx,float ty,float tz)
{
	m[ 0] = 1.0; m[ 4] = 0.0; m[ 8] = 0.0; m[12] =  tx;
	m[ 1] = 0.0; m[ 5] = 1.0; m[ 9] = 0.0; m[13] =  ty;
	m[ 2] = 0.0; m[ 6] = 0.0; m[10] = 1.0; m[14] =  tz;
	m[ 3] = 0.0; m[ 7] = 0.0; m[11] = 0.0; m[15] = 1.0;
}

// -----------------------------------------------------------
/* MATRIX SET TRANSLATE */
// -----------------------------------------------------------
void matrix_concatenate_translate(matrix m,float tx,float ty,float tz)
{
	m[12] =  m[0]*tx + m[4]*ty + m[8]*tz  + m[12]; 
	m[13] =  m[1]*tx + m[5]*ty + m[9]*tz  + m[13]; 
	m[14] =  m[2]*tx + m[6]*ty + m[10]*tz + m[14];
	m[15] =  m[3]*tx + m[7]*ty + m[11]*tz + m[15];
}

// -----------------------------------------------------------
/* MATRIX SET X ROTATE */
// -----------------------------------------------------------
void matrix_set_x_rotate(matrix m,float angle)
{
	double radians,ca,sa;
	
	radians = angle*3.1415926535/180;
	
	ca = cos(radians);
	sa = sin(radians);
	
	m[ 0] = 1.0; m[ 4] = 0.0; m[ 8] = 0.0; m[12] = 0.0;
	m[ 1] = 0.0; m[ 5] =  ca; m[ 9] = -sa; m[13] = 0.0;
	m[ 2] = 0.0; m[ 6] =  sa; m[10] =  ca; m[14] = 0.0;
	m[ 3] = 0.0; m[ 7] = 0.0; m[11] = 0.0; m[15] = 1.0;
}

// -----------------------------------------------------------
/* MATRIX SET Y ROTATE */
// -----------------------------------------------------------
void matrix_set_y_rotate(matrix m,float angle)
{
	float radians;
	
	radians = angle*3.1415926535/180;
	m[ 0] = cos(radians);  m[ 4] = 0.0; m[ 8] = sin(radians); m[12] = 0.0;
	m[ 1] = 0.0;           m[ 5] = 1.0; m[ 9] = 0.0;          m[13] = 0.0;
	m[ 2] = -sin(radians); m[ 6] = 0.0; m[10] = cos(radians); m[14] = 0.0;
	m[ 3] = 0.0;           m[ 7] = 0.0; m[11] = 0.0;          m[15] = 1.0;
}

// -----------------------------------------------------------
/* MATRIX SET Z ROTATE */
// -----------------------------------------------------------
void matrix_set_z_rotate(matrix m,float angle)
{
	float radians;
	
	radians = angle*3.1415926535/180;
	m[ 0] = cos(radians); m[ 4] = -sin(radians); m[ 8] = 0.0; m[12] = 0.0;
	m[ 1] = sin(radians); m[ 5] =  cos(radians); m[ 9] = 0.0; m[13] = 0.0;
	m[ 2] = 0.0;          m[ 6] = 0.0;           m[10] = 1.0; m[14] = 0.0;
	m[ 3] = 0.0;          m[ 7] = 0.0;           m[11] = 0.0; m[15] = 1.0;
}


// -----------------------------------------------------------
/* MATRIX MULTIPLY A = B.C */
// -----------------------------------------------------------
void matrix_multiply(matrix a,matrix b,matrix c)
{
	matrix	t;
	int		i,j,k;
	
	for (i=0; i<4; i++) {
		for (j=0; j<4; j++) {
			t[4*i+j] = 0.0;
			for (k=0; k<4; k++) {
				t[4*i+j] += b[4*i+k]*c[4*k+j];
			}
		}
	}
	for (i=0; i<16; i++) a[i] = t[i];
}

// -----------------------------------------------------------
// PRINT MATRIX 
// -----------------------------------------------------------
void matrix_print(float **A)
{
	int	i,j;
	
	for (i=0; i<4; i++) {
		for (j=0; j<4; j++) {
			printf("%f ",A[i][j]);
		}
		printf("\n");
	}
}

// ================================================================================
// NxM matrix routines
// ================================================================================
	
// -------------------------------------------------------
// MATRIX NxM ALLOCATE
// -------------------------------------------------------
matrixNxM matrix_allocate_NxM(int n,int m)
{
	int			i;
	matrixNxM	m1;
	
	m1 = (float **)malloc(sizeof(float *)*n);
	for (i=0; i<n; i++) m1[i] = (float *)malloc(sizeof(float)*m);
	return m1;
}

// -----------------------------------------------------------
// MATRIX MULTIPLY NxMxP: A = B.C 
// A is nXp; B is nXm; C is mXp
// -----------------------------------------------------------
void matrixNxMxP_multiply(matrixNxM a,matrixNxM b,matrixNxM c,int n, int m,int p)
{
	matrixNxM	t;
	int			i,j,k;
	
	t = (float **)malloc(sizeof(float *)*n);
	for (i=0; i<n; i++) t[i] = (float *)malloc(sizeof(float)*p);
	
	for (i=0; i<n; i++) {
		for (j=0; j<p; j++) {
			t[i][j] = 0.0;
			for (k=0; k<m; k++) {
				t[i][j] += b[i][k]*c[k][j];
			}
		}
	}
	for (i=0; i<n; i++)  for (j=0; j<p; j++) a[i][j] = t[i][j];
}

// -----------------------------------------------------------
// MATRIX MULTIPLY VECTOR NxM: A = B.C 
// A is n; B is nXm; C is m
// -----------------------------------------------------------
void matrixNxM_multiply_vector(float *a,matrixNxM b,float *c,int n, int m)
{
	float		*t;
	int			i,j;
	
	t = (float *)malloc(sizeof(float)*n);
	
	for (i=0; i<n; i++) {
		t[i] = 0.0;
		for (j=0; j<m; j++) {
				t[i] += b[i][j]*c[j];
		}
	}
	for (i=0; i<n; i++)  a[i] = t[i];
}

/* --------------------------------------------------------------------------- */
/* PRINT MATRIX NxM */
/* --------------------------------------------------------------------------- */
void matrixNxM_print(matrixNxM A,int n,int m)
{
	int	i,j;
	
	for (i=0; i<n; i++) {
		for (j=0; j<m; j++) {
			printf("%f ",A[i][j]);
		}
		printf("\n");
	}
}	
#endif
