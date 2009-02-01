#ifndef __KGUIMATRIX__
#define __KGUIMATRIX__

class kGUIVector3
{
public:
	double m_x,m_y,m_z;
};

class kGUIMatrix33
{
public:
	kGUIMatrix33() {Identity();}
	void Identity(void);
	void SetRotX(double angle);			/* angle = 0.0 to 1.0f */
	void SetRotY(double angle);			/* angle = 0.0 to 1.0f */
	void SetRotZ(double angle);			/* angle = 0.0 to 1.0f */
	void SetTX(double x) {m_t.m_x=x;}
	void SetTY(double y) {m_t.m_y=y;}
	void SetTZ(double z) {m_t.m_z=z;}
	void SetScale(double scale);		/* sets scale for x,y & z */
	void Transform(kGUIVector3 *in,kGUIVector3 *out);
	void Mult(kGUIMatrix33 *in,kGUIMatrix33 *out);
private:
	double m_m[3][3];
	kGUIVector3 m_t;
};

#endif
