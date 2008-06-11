#ifndef __KGUIPROT__
#define __KGUIPROT__

//class kGUIProt : public kGUIRandom
class kGUIProt
{
public:
	kGUIProt();
	~kGUIProt();
	bool SetKey(const char *fn,unsigned int keystart,unsigned int keylen,bool load);
	void SetMemKey(const char *buf,long keysize,unsigned int keystart,unsigned int keylen);
	const char *EncryptToHex(const char *in,int insize, int *outlen);
	const char *DecryptFromHex(const char *in,int insize, int *outlen);
	const unsigned char *Encrypt(const unsigned char *in,long insize);
	const unsigned char *Decrypt(const unsigned char *in,long insize);
private:
	bool LoadSeed(kGUIRandom *r,unsigned int offset);
	kGUIString m_keyfilename;
	unsigned int m_keystart;
	unsigned int m_keylen;
	const char *m_keyfile;
	unsigned long m_keyfilesize;
};

#endif
