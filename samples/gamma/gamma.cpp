/*********************************************************************************/
/* Gamma - kGUI sample program for calculating your system/monitor gamma value   */
/*                                                                               */
/* Programmed by Kevin Pickell                                                   */
/* 15-Jun-2008                                                                   */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"

#define APPNAME "Gamma"

#if defined(WIN32) || defined(MINGW)
/* this is for the ICON in windows */
#include "resource.h"
#endif

/* this includes the main loop for the selected OS, like Windows, Linux, Mac etc */
#include "kguisys.cpp"

class GammaHorizBox : public kGUIObj
{
public:
	void Draw(void);
	bool UpdateInput(void) {return false;}
private:
};

class GammaVertBox : public kGUIObj
{
public:
	void Draw(void);
	bool UpdateInput(void) {return false;}
private:
};

class GammaHalfBox : public kGUIObj
{
public:
	void Draw(void);
	bool UpdateInput(void) {return false;}
private:
};


class GammaTextBox : public kGUIObj
{
public:
	void Draw(void);
	bool UpdateInput(void) {return false;}
private:
};


class GammaSample
{
public:
	GammaSample();
	~GammaSample() {}
private:
	CALLBACKGLUEPTR(GammaSample,GammaEvent,kGUIEvent);		/* make a static connection to the callback */
	void GammaEvent(kGUIEvent *event);

	kGUIScrollTextObj m_gamma;

	GammaHorizBox m_hbox;
	GammaVertBox m_vbox;
	GammaHalfBox m_hbox2;
	GammaTextBox m_tbox;
};

GammaSample *g_GammaSample;

void AppInit(void)
{
	kGUI::LoadFont("font.ttf");	/* use default font inside kgui */
	kGUI::SetDefFontSize(11);
	kGUI::SetDefReportFontSize(11);

	g_GammaSample=new GammaSample();
}

void AppClose(void)
{
	delete g_GammaSample;
}

GammaSample::GammaSample()
{
	kGUIWindowObj *background;

	/* get pointer to the background window object */
	background=kGUI::GetBackground();
 	background->SetTitle("GammaSample");

	m_gamma.SetPos(25,25);
	m_gamma.SetSize(200,35);
	m_gamma.SetString("2.2");
	background->AddObject(&m_gamma);

	m_hbox.SetPos(20,48);
	m_hbox.SetSize(256,256);
	background->AddObject(&m_hbox);

	m_vbox.SetPos(40+256,48);
	m_vbox.SetSize(256,256);
	background->AddObject(&m_vbox);

	m_tbox.SetPos(80+512,48);
	m_tbox.SetSize(256,256);
	background->AddObject(&m_tbox);

	m_hbox2.SetPos(120+768,48);
	m_hbox2.SetSize(256,256);
	background->AddObject(&m_hbox2);

	m_gamma.SetEventHandler(this,CALLBACKNAME(GammaEvent));
}

/* you can have a unique event handler for each object, or you can have one to handle many objects */
void GammaSample::GammaEvent(kGUIEvent *event)
{
	double g;

	switch(event->GetEvent())
	{
	case EVENT_PRESSED:
		g=m_gamma.GetDouble();
		if(event->m_value[0].i==-1)
			g-=0.01f;
		else if(event->m_value[0].i==1)
			g+=0.01f;
		m_gamma.Sprintf("%.2f",g);
		kGUI::m_subpixcollector.SetGamma(g);
		kGUI::GetBackground()->Dirty();
	break;
	}
}

void GammaHorizBox::Draw(void)
{
	kGUICorners c;
	int w,h;
	int x,y;
	double dx,xstep;

	GetCorners(&c);
	kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(255,255,255));
	kGUI::m_subpixcollector.SetColor(DrawColor(0,0,0),1.0f);
	w=c.rx-c.lx;
	h=c.by-c.ty;
	kGUI::m_subpixcollector.SetBounds(c.ty,c.by);
	xstep=1.0f/(double)w;
	for(x=0;x<w;x+=2)
	{
		dx=(double)(c.lx+x);
		for(y=0;y<h;++y)
		{
			kGUI::m_subpixcollector.AddRect(dx,(double)y+c.ty,1.0f,1.0f,1.0f);
			dx+=xstep;
		}
	}
	kGUI::m_subpixcollector.Draw();
}

void GammaVertBox::Draw(void)
{
	kGUICorners c;
	int w,h;
	int x,y;
	double dy,ystep;

	GetCorners(&c);
	kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(255,255,255));
	kGUI::m_subpixcollector.SetColor(DrawColor(0,0,0),1.0f);
	w=c.rx-c.lx;
	h=c.by-c.ty;
	kGUI::m_subpixcollector.SetBounds(c.ty,c.by);
	ystep=1.0f/(double)h;
	for(y=0;y<h;y+=2)
	{
		dy=(double)(c.ty+y);
		for(x=0;x<w;++x)
		{
			kGUI::m_subpixcollector.AddRect((double)x+c.lx,dy,1.0f,1.0f,1.0f);
			dy+=ystep;
		}
	}
	kGUI::m_subpixcollector.Draw();
}

void GammaTextBox::Draw(void)
{
	double x,y;
	kGUICorners c;
	kGUIText t;

	GetCorners(&c);
	kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(255,255,255));

	y=(double)c.ty;
	x=(double)c.lx;
	t.SetFontSize(9);
	t.SetString("Hello World!!!!!!!!!!!");
	do
	{
		t.DrawRot(x,y,0.0f,DrawColor(0,0,0));
		y+=t.GetHeight();
		x+=0.25f;
	}while(y<(double)c.by);
}

void GammaHalfBox::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(128,128,128));
}
