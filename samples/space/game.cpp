
/* vector model data is here */

GameVector3 g_shippoints[]={
	{-10.0f,10.0f,0.0f},
	{0.0f,-10.0f,0.0f},
	{10.0f,10.0f,0.0f},
	{0.0f,5.0f,0.0f}};

GameModel g_shipmodel={sizeof(g_shippoints)/sizeof(g_shippoints[0]),g_shippoints};

GameVector3 g_exshippoints1[]={
	{-10.0f,10.0f,0.0f},
	{0.0f,-10.0f,0.0f}};

GameVector3 g_exshippoints2[]={
	{0.0f,-10.0f,0.0f},
	{10.0f,10.0f,0.0f}};

GameVector3 g_exshippoints3[]={
	{10.0f,10.0f,0.0f},
	{0.0f,5.0f,0.0f}};

GameVector3 g_exshippoints4[]={
	{0.0f,5.0f,0.0f},
	{-10.0f,10.0f,0.0f}};

GameModel g_exshipmodels[4]={
	{sizeof(g_exshippoints1)/sizeof(g_exshippoints1[0]),g_exshippoints1},
	{sizeof(g_exshippoints2)/sizeof(g_exshippoints2[0]),g_exshippoints2},
	{sizeof(g_exshippoints3)/sizeof(g_exshippoints3[0]),g_exshippoints3},
	{sizeof(g_exshippoints4)/sizeof(g_exshippoints4[0]),g_exshippoints4}};

GameVector3 g_shotpoints[]={
	{-2.0f,2.0f,0.0f},
	{0.0f,-2.0f,0.0f},
	{2.0f,2.0f,0.0f},
	{0.0f,1.0f,0.0f}};

GameModel g_shotmodel[]={{sizeof(g_shotpoints)/sizeof(g_shotpoints[0]),g_shotpoints}};

GameVector3 g_shipthrust1[]={
	{-5.0f,10.0f,0.0f},
	{ 5.0f,10.0f,0.0f},
	{ 0.0f,15.0f,0.0f}};

GameVector3 g_shipthrust2[]={
	{-5.0f,10.0f,0.0f},
	{ 5.0f,10.0f,0.0f},
	{ 0.0f,18.0f,0.0f}};

GameVector3 g_shipthrust3[]={
	{-5.0f,10.0f,0.0f},
	{ 5.0f,10.0f,0.0f},
	{ 0.0f,22.0f,0.0f}};

GameVector3 g_shipthrust4[]={
	{-5.0f,10.0f,0.0f},
	{ 5.0f,10.0f,0.0f},
	{ 0.0f,18.0f,0.0f}};

GameModel g_thrustmodel[4]={
		{sizeof(g_shipthrust1)/sizeof(g_shipthrust1[0]),g_shipthrust1},
		{sizeof(g_shipthrust2)/sizeof(g_shipthrust2[0]),g_shipthrust2},
		{sizeof(g_shipthrust3)/sizeof(g_shipthrust3[0]),g_shipthrust3},
		{sizeof(g_shipthrust4)/sizeof(g_shipthrust4[0]),g_shipthrust4}};

GameVector3 g_ufopoints[]={
	{(20.0f-25.0f)*1.0f,(0.0f-15.0f)*1.0f,0.0f},
	{(30.0f-25.0f)*1.0f,(0.0f-15.0f)*1.0f,0.0f},
	{(35.0f-25.0f)*1.0f,(10.0f-15.0f)*1.0f,0.0f},
	{(50.0f-25.0f)*1.0f,(20.0f-15.0f)*1.0f,0.0f},
	{(35.0f-25.0f)*1.0f,(30.0f-15.0f)*1.0f,0.0f},
	{(15.0f-25.0f)*1.0f,(30.0f-15.0f)*1.0f,0.0f},
	{(0.0f-25.0f)*1.0f,(20.0f-15.0f)*1.0f,0.0f},
	{(15.0f-25.0f)*1.0f,(10.0f-15.0f)*1.0f,0.0f},
	{(20.0f-25.0f)*1.0f,(0.0f-15.0f)*1.0f,1.0f},
	{(15.0f-25.0f)*1.0f,(10.0f-15.0f)*1.0f,0.0f},
	{(35.0f-25.0f)*1.0f,(10.0f-15.0f)*1.0f,1.0f},
	{(0.0f-25.0f)*1.0f,(20.0f-15.0f)*1.0f,0.0f},
	{(50.0f-25.0f)*1.0f,(20.0f-15.0f)*1.0f,1.0f}};

GameModel g_ufomodel={sizeof(g_ufopoints)/sizeof(g_ufopoints[0]),g_ufopoints};

GameVector3 g_rockpoints11[]={
	{(30.0f-40.0f)*1.0f,(0.0f-36.0f)*1.0f,0.0f},
	{(59.0f-40.0f)*1.0f,(0.0f-36.0f)*1.0f,0.0f},
	{(80.0f-40.0f)*1.0f,(28.0f-36.0f)*1.0f,0.0f},
	{(80.0f-40.0f)*1.0f,(46.0f-36.0f)*1.0f,0.0f},
	{(60.0f-40.0f)*1.0f,(72.0f-36.0f)*1.0f,0.0f},
	{(40.0f-40.0f)*1.0f,(72.0f-36.0f)*1.0f,0.0f},
	{(39.0f-40.0f)*1.0f,(47.0f-36.0f)*1.0f,0.0f},
	{(19.0f-40.0f)*1.0f,(71.0f-36.0f)*1.0f,0.0f},
	{(0.0f-40.0f)*1.0f,(46.0f-36.0f)*1.0f,0.0f},
	{(19.0f-40.0f)*1.0f,(38.0f-36.0f)*1.0f,0.0f},
	{(0.0f-40.0f)*1.0f,(29.0f-36.0f)*1.0f,0.0f}};

GameVector3 g_rockpoints12[]={
	{(24.0f-40.0f)*1.0f,(0.0f-36.0f)*1.0f,0.0f},
	{(44.0f-40.0f)*1.0f,(16.0f-36.0f)*1.0f,0.0f},
	{(63.0f-40.0f)*1.0f,(0.0f-36.0f)*1.0f,0.0f},
	{(80.0f-40.0f)*1.0f,(17.0f-36.0f)*1.0f,0.0f},
	{(72.0f-40.0f)*1.0f,(36.0f-36.0f)*1.0f,0.0f},
	{(80.0f-40.0f)*1.0f,(50.0f-36.0f)*1.0f,0.0f},
	{(54.0f-40.0f)*1.0f,(71.0f-36.0f)*1.0f,0.0f},
	{(24.0f-40.0f)*1.0f,(71.0f-36.0f)*1.0f,0.0f},
	{(0.0f-40.0f)*1.0f,(50.0f-36.0f)*1.0f,0.0f},
	{(0.0f-40.0f)*1.0f,(15.0f-36.0f)*1.0f,0.0f}};

GameVector3 g_rockpoints13[]={
	{(24.0f-40.0f)*1.0f,(0.0f-36.0f)*1.0f,0.0f},
	{(42.0f-40.0f)*1.0f,(12.0f-36.0f)*1.0f,0.0f},
	{(60.0f-40.0f)*1.0f,(0.0f-36.0f)*1.0f,0.0f},
	{(80.0f-40.0f)*1.0f,(19.0f-36.0f)*1.0f,0.0f},
	{(61.0f-40.0f)*1.0f,(28.0f-36.0f)*1.0f,0.0f},
	{(80.0f-40.0f)*1.0f,(46.0f-36.0f)*1.0f,0.0f},
	{(60.0f-40.0f)*1.0f,(70.0f-36.0f)*1.0f,0.0f},
	{(34.0f-40.0f)*1.0f,(60.0f-36.0f)*1.0f,0.0f},
	{(24.0f-40.0f)*1.0f,(73.0f-36.0f)*1.0f,0.0f},
	{(2.0f-40.0f)*1.0f,(52.0f-36.0f)*1.0f,0.0f},
	{(13.0f-40.0f)*1.0f,(36.0f-36.0f)*1.0f,0.0f},
	{(0.0f-40.0f)*1.0f,(18.0f-36.0f)*1.0f,0.0f}};

GameVector3 g_rockpoints14[]={
	{(22.0f-40.0f)*1.0f,(0.0f-36.0f)*1.0f,0.0f},
	{(51.0f-40.0f)*1.0f,(0.0f-36.0f)*1.0f,0.0f},
	{(80.0f-40.0f)*1.0f,(17.0f-36.0f)*1.0f,0.0f},
	{(80.0f-40.0f)*1.0f,(28.0f-36.0f)*1.0f,0.0f},
	{(53.0f-40.0f)*1.0f,(38.0f-36.0f)*1.0f,0.0f},
	{(80.0f-40.0f)*1.0f,(54.0f-36.0f)*1.0f,0.0f},
	{(60.0f-40.0f)*1.0f,(74.0f-36.0f)*1.0f,0.0f},
	{(48.0f-40.0f)*1.0f,(63.0f-36.0f)*1.0f,0.0f},
	{(23.0f-40.0f)*1.0f,(74.0f-36.0f)*1.0f,0.0f},
	{(0.0f-40.0f)*1.0f,(48.0f-36.0f)*1.0f,0.0f},
	{(3.0f-40.0f)*1.0f,(19.0f-36.0f)*1.0f,0.0f},
	{(30.0f-40.0f)*1.0f,(18.0f-36.0f)*1.0f,0.0f}};

GameVector3 g_rockpoints21[]={
	{(30.0f-40.0f)*0.66f,(0.0f-36.0f)*0.66f,0.0f},
	{(59.0f-40.0f)*0.66f,(0.0f-36.0f)*0.66f,0.0f},
	{(80.0f-40.0f)*0.66f,(28.0f-36.0f)*0.66f,0.0f},
	{(80.0f-40.0f)*0.66f,(46.0f-36.0f)*0.66f,0.0f},
	{(60.0f-40.0f)*0.66f,(72.0f-36.0f)*0.66f,0.0f},
	{(40.0f-40.0f)*0.66f,(72.0f-36.0f)*0.66f,0.0f},
	{(39.0f-40.0f)*0.66f,(47.0f-36.0f)*0.66f,0.0f},
	{(19.0f-40.0f)*0.66f,(71.0f-36.0f)*0.66f,0.0f},
	{(0.0f-40.0f)*0.66f,(46.0f-36.0f)*0.66f,0.0f},
	{(19.0f-40.0f)*0.66f,(38.0f-36.0f)*0.66f,0.0f},
	{(0.0f-40.0f)*0.66f,(29.0f-36.0f)*0.66f,0.0f}};

GameVector3 g_rockpoints22[]={
	{(24.0f-40.0f)*0.66f,(0.0f-36.0f)*0.66f,0.0f},
	{(44.0f-40.0f)*0.66f,(16.0f-36.0f)*0.66f,0.0f},
	{(63.0f-40.0f)*0.66f,(0.0f-36.0f)*0.66f,0.0f},
	{(80.0f-40.0f)*0.66f,(17.0f-36.0f)*0.66f,0.0f},
	{(72.0f-40.0f)*0.66f,(36.0f-36.0f)*0.66f,0.0f},
	{(80.0f-40.0f)*0.66f,(50.0f-36.0f)*0.66f,0.0f},
	{(54.0f-40.0f)*0.66f,(71.0f-36.0f)*0.66f,0.0f},
	{(24.0f-40.0f)*0.66f,(71.0f-36.0f)*0.66f,0.0f},
	{(0.0f-40.0f)*0.66f,(50.0f-36.0f)*0.66f,0.0f},
	{(0.0f-40.0f)*0.66f,(15.0f-36.0f)*0.66f,0.0f}};

GameVector3 g_rockpoints23[]={
	{(24.0f-40.0f)*0.66f,(0.0f-36.0f)*0.66f,0.0f},
	{(42.0f-40.0f)*0.66f,(12.0f-36.0f)*0.66f,0.0f},
	{(60.0f-40.0f)*0.66f,(0.0f-36.0f)*0.66f,0.0f},
	{(80.0f-40.0f)*0.66f,(19.0f-36.0f)*0.66f,0.0f},
	{(61.0f-40.0f)*0.66f,(28.0f-36.0f)*0.66f,0.0f},
	{(80.0f-40.0f)*0.66f,(46.0f-36.0f)*0.66f,0.0f},
	{(60.0f-40.0f)*0.66f,(70.0f-36.0f)*0.66f,0.0f},
	{(34.0f-40.0f)*0.66f,(60.0f-36.0f)*0.66f,0.0f},
	{(24.0f-40.0f)*0.66f,(73.0f-36.0f)*0.66f,0.0f},
	{(2.0f-40.0f)*0.66f,(52.0f-36.0f)*0.66f,0.0f},
	{(13.0f-40.0f)*0.66f,(36.0f-36.0f)*0.66f,0.0f},
	{(0.0f-40.0f)*0.66f,(18.0f-36.0f)*0.66f,0.0f}};

GameVector3 g_rockpoints24[]={
	{(22.0f-40.0f)*0.66f,(0.0f-36.0f)*0.66f,0.0f},
	{(51.0f-40.0f)*0.66f,(0.0f-36.0f)*0.66f,0.0f},
	{(80.0f-40.0f)*0.66f,(17.0f-36.0f)*0.66f,0.0f},
	{(80.0f-40.0f)*0.66f,(28.0f-36.0f)*0.66f,0.0f},
	{(53.0f-40.0f)*0.66f,(38.0f-36.0f)*0.66f,0.0f},
	{(80.0f-40.0f)*0.66f,(54.0f-36.0f)*0.66f,0.0f},
	{(60.0f-40.0f)*0.66f,(74.0f-36.0f)*0.66f,0.0f},
	{(48.0f-40.0f)*0.66f,(63.0f-36.0f)*0.66f,0.0f},
	{(23.0f-40.0f)*0.66f,(74.0f-36.0f)*0.66f,0.0f},
	{(0.0f-40.0f)*0.66f,(48.0f-36.0f)*0.66f,0.0f},
	{(3.0f-40.0f)*0.66f,(19.0f-36.0f)*0.66f,0.0f},
	{(30.0f-40.0f)*0.66f,(18.0f-36.0f)*0.66f,0.0f}};

GameVector3 g_rockpoints31[]={
	{(30.0f-40.0f)*0.33f,(0.0f-36.0f)*0.33f,0.0f},
	{(59.0f-40.0f)*0.33f,(0.0f-36.0f)*0.33f,0.0f},
	{(80.0f-40.0f)*0.33f,(28.0f-36.0f)*0.33f,0.0f},
	{(80.0f-40.0f)*0.33f,(46.0f-36.0f)*0.33f,0.0f},
	{(60.0f-40.0f)*0.33f,(72.0f-36.0f)*0.33f,0.0f},
	{(40.0f-40.0f)*0.33f,(72.0f-36.0f)*0.33f,0.0f},
	{(39.0f-40.0f)*0.33f,(47.0f-36.0f)*0.33f,0.0f},
	{(19.0f-40.0f)*0.33f,(71.0f-36.0f)*0.33f,0.0f},
	{(0.0f-40.0f)*0.33f,(46.0f-36.0f)*0.33f,0.0f},
	{(19.0f-40.0f)*0.33f,(38.0f-36.0f)*0.33f,0.0f},
	{(0.0f-40.0f)*0.33f,(29.0f-36.0f)*0.33f,0.0f}};

GameVector3 g_rockpoints32[]={
	{(24.0f-40.0f)*0.33f,(0.0f-36.0f)*0.33f,0.0f},
	{(44.0f-40.0f)*0.33f,(16.0f-36.0f)*0.33f,0.0f},
	{(63.0f-40.0f)*0.33f,(0.0f-36.0f)*0.33f,0.0f},
	{(80.0f-40.0f)*0.33f,(17.0f-36.0f)*0.33f,0.0f},
	{(72.0f-40.0f)*0.33f,(36.0f-36.0f)*0.33f,0.0f},
	{(80.0f-40.0f)*0.33f,(50.0f-36.0f)*0.33f,0.0f},
	{(54.0f-40.0f)*0.33f,(71.0f-36.0f)*0.33f,0.0f},
	{(24.0f-40.0f)*0.33f,(71.0f-36.0f)*0.33f,0.0f},
	{(0.0f-40.0f)*0.33f,(50.0f-36.0f)*0.33f,0.0f},
	{(0.0f-40.0f)*0.33f,(15.0f-36.0f)*0.33f,0.0f}};

GameVector3 g_rockpoints33[]={
	{(24.0f-40.0f)*0.33f,(0.0f-36.0f)*0.33f,0.0f},
	{(42.0f-40.0f)*0.33f,(12.0f-36.0f)*0.33f,0.0f},
	{(60.0f-40.0f)*0.33f,(0.0f-36.0f)*0.33f,0.0f},
	{(80.0f-40.0f)*0.33f,(19.0f-36.0f)*0.33f,0.0f},
	{(61.0f-40.0f)*0.33f,(28.0f-36.0f)*0.33f,0.0f},
	{(80.0f-40.0f)*0.33f,(46.0f-36.0f)*0.33f,0.0f},
	{(60.0f-40.0f)*0.33f,(70.0f-36.0f)*0.33f,0.0f},
	{(34.0f-40.0f)*0.33f,(60.0f-36.0f)*0.33f,0.0f},
	{(24.0f-40.0f)*0.33f,(73.0f-36.0f)*0.33f,0.0f},
	{(2.0f-40.0f)*0.33f,(52.0f-36.0f)*0.33f,0.0f},
	{(13.0f-40.0f)*0.33f,(36.0f-36.0f)*0.33f,0.0f},
	{(0.0f-40.0f)*0.33f,(18.0f-36.0f)*0.33f,0.0f}};

GameVector3 g_rockpoints34[]={
	{(22.0f-40.0f)*0.33f,(0.0f-36.0f)*0.33f,0.0f},
	{(51.0f-40.0f)*0.33f,(0.0f-36.0f)*0.33f,0.0f},
	{(80.0f-40.0f)*0.33f,(17.0f-36.0f)*0.33f,0.0f},
	{(80.0f-40.0f)*0.33f,(28.0f-36.0f)*0.33f,0.0f},
	{(53.0f-40.0f)*0.33f,(38.0f-36.0f)*0.33f,0.0f},
	{(80.0f-40.0f)*0.33f,(54.0f-36.0f)*0.33f,0.0f},
	{(60.0f-40.0f)*0.33f,(74.0f-36.0f)*0.33f,0.0f},
	{(48.0f-40.0f)*0.33f,(63.0f-36.0f)*0.33f,0.0f},
	{(23.0f-40.0f)*0.33f,(74.0f-36.0f)*0.33f,0.0f},
	{(0.0f-40.0f)*0.33f,(48.0f-36.0f)*0.33f,0.0f},
	{(3.0f-40.0f)*0.33f,(19.0f-36.0f)*0.33f,0.0f},
	{(30.0f-40.0f)*0.33f,(18.0f-36.0f)*0.33f,0.0f}};

GameModel g_rockmodels[NUMROCKSIZES*NUMROCKMODELS]={
	{sizeof(g_rockpoints11)/sizeof(g_rockpoints11[0]),g_rockpoints11},
	{sizeof(g_rockpoints12)/sizeof(g_rockpoints12[0]),g_rockpoints12},
	{sizeof(g_rockpoints13)/sizeof(g_rockpoints13[0]),g_rockpoints13},
	{sizeof(g_rockpoints14)/sizeof(g_rockpoints14[0]),g_rockpoints14},
	{sizeof(g_rockpoints21)/sizeof(g_rockpoints21[0]),g_rockpoints21},
	{sizeof(g_rockpoints22)/sizeof(g_rockpoints22[0]),g_rockpoints22},
	{sizeof(g_rockpoints23)/sizeof(g_rockpoints23[0]),g_rockpoints23},
	{sizeof(g_rockpoints24)/sizeof(g_rockpoints24[0]),g_rockpoints24},
	{sizeof(g_rockpoints31)/sizeof(g_rockpoints31[0]),g_rockpoints31},
	{sizeof(g_rockpoints32)/sizeof(g_rockpoints32[0]),g_rockpoints32},
	{sizeof(g_rockpoints33)/sizeof(g_rockpoints33[0]),g_rockpoints33},
	{sizeof(g_rockpoints34)/sizeof(g_rockpoints34[0]),g_rockpoints34}};

GameColor GameBase::m_shipcolors[MAXPLAYERS]={
	DrawColor(255,0,0),
	DrawColor(0,255,0),
	DrawColor(0,0,255),
#if MAXPLAYERS==8
	DrawColor(255,255,0),
	DrawColor(255,0,255),
	DrawColor(0,255,255),
	DrawColor(128,255,255),
#endif
	DrawColor(255,255,255)};

GameVector3 g_decpoints[]={
	{6.0f,16.0f,0.0f},
	{10.0f,16.0f,1.0f}};

GameModel g_decmodel={sizeof(g_decpoints)/sizeof(g_decpoints[0]),g_decpoints};

GameVector3 g_0points[]={
	{0.0f,16.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{16.0f,0.0f,0.0f},
	{16.0f,16.0f,0.0f}};

GameModel g_0model={sizeof(g_0points)/sizeof(g_0points[0]),g_0points};

GameVector3 g_1points[]={
	{16.0f,0.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_1model={sizeof(g_1points)/sizeof(g_1points[0]),g_1points};

GameVector3 g_2points[]={
	{0.0f,0.0f,0.0f},
	{16.0f,0.0f,0.0f},
	{16.0f,8.0f,0.0f},
	{0.0f,8.0f,0.0f},
	{0.0f,16.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_2model={sizeof(g_2points)/sizeof(g_2points[0]),g_2points};

GameVector3 g_3points[]={
	{0.0f,0.0f,0.0f},
	{16.0f,0.0f,0.0f},
	{16.0f,8.0f,0.0f},
	{0.0f,8.0f,1.0f},
	{16.0f,8.0f,0.0f},
	{16.0f,16.0f,0.0f},
	{0.0f,16.0f,1.0f}};

GameModel g_3model={sizeof(g_3points)/sizeof(g_3points[0]),g_3points};

GameVector3 g_4points[]={
	{0.0f,0.0f,0.0f},
	{0.0f,8.0f,0.0f},
	{16.0f,8.0f,0.0f},
	{16.0f,0.0f,1.0f},
	{16.0f,8.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_4model={sizeof(g_4points)/sizeof(g_4points[0]),g_4points};

GameVector3 g_5points[]={
	{16.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,8.0f,0.0f},
	{16.0f,8.0f,0.0f},
	{16.0f,16.0f,0.0f},
	{0.0f,16.0f,1.0f}};

GameModel g_5model={sizeof(g_5points)/sizeof(g_5points[0]),g_5points};

GameVector3 g_6points[]={
	{0.0f,0.0f,0.0f},
	{0.0f,16.0f,0.0f},
	{16.0f,16.0f,0.0f},
	{16.0f,8.0f,0.0f},
	{0.0f,8.0f,1.0f}};

GameModel g_6model={sizeof(g_6points)/sizeof(g_6points[0]),g_6points};

GameVector3 g_7points[]={
	{0.0f,0.0f,0.0f},
	{16.0f,0.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_7model={sizeof(g_7points)/sizeof(g_7points[0]),g_7points};

GameVector3 g_8points[]={
	{0.0f,0.0f,0.0f},
	{0.0f,16.0f,0.0f},
	{16.0f,16.0f,0.0f},
	{16.0f,0.0f,0.0f},
	{0.0f,0.0f,1.0f},
	{0.0f,8.0f,0.0f},
	{16.0f,8.0f,1.0f}};

GameModel g_8model={sizeof(g_8points)/sizeof(g_8points[0]),g_8points};

GameVector3 g_9points[]={
	{16.0f,16.0f,0.0f},
	{16.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,8.0f,0.0f},
	{16.0f,8.0f,1.0f}};

GameModel g_9model={sizeof(g_9points)/sizeof(g_9points[0]),g_9points};

GameVector3 g_apoints[]={
	{0.0f,16.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{16.0f,0.0f,0.0f},
	{16.0f,16.0f,1.0f},
	{0.0f,8.0f,0.0f},
	{16.0f,8.0f,1.0f}};

GameModel g_amodel={sizeof(g_apoints)/sizeof(g_apoints[0]),g_apoints};

GameVector3 g_bpoints[]={
	{0.0f,16.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{14.0f,0.0f,0.0f},
	{16.0f,2.0f,0.0f},
	{16.0f,2.0f,0.0f},
	{16.0f,6.0f,0.0f},
	{14.0f,8.0f,0.0f},
	{16.0f,10.0f,0.0f},
	{16.0f,14.0f,0.0f},
	{14.0f,16.0f,0.0f},
	{0.0f,16.0f,1.0f},
	{0.0f,8.0f,0.0f},
	{14.0f,8.0f,1.0f}};

GameModel g_bmodel={sizeof(g_bpoints)/sizeof(g_bpoints[0]),g_bpoints};

GameVector3 g_cpoints[]={
	{16.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,16.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_cmodel={sizeof(g_cpoints)/sizeof(g_cpoints[0]),g_cpoints};

GameVector3 g_dpoints[]={
	{0.0f,0.0f,0.0f},
	{14.0f,0.0f,0.0f},
	{16.0f,2.0f,0.0f},
	{16.0f,14.0f,0.0f},
	{14.0f,16.0f,0.0f},
	{0.0f,16.0f,0.0f}};

GameModel g_dmodel={sizeof(g_dpoints)/sizeof(g_dpoints[0]),g_dpoints};

GameVector3 g_epoints[]={
	{16.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,16.0f,0.0f},
	{16.0f,16.0f,1.0f},
	{0.0f,8.0f,0.0f},
	{14.0f,8.0f,1.0f}};

GameModel g_emodel={sizeof(g_epoints)/sizeof(g_epoints[0]),g_epoints};

GameVector3 g_fpoints[]={
	{16.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,16.0f,1.0f},
	{0.0f,8.0f,0.0f},
	{14.0f,8.0f,1.0f}};

GameModel g_fmodel={sizeof(g_fpoints)/sizeof(g_fpoints[0]),g_fpoints};

GameVector3 g_gpoints[]={
	{16.0f,0.0f,0.0f},
	{2.0f,0.0f,0.0f},
	{0.0f,2.0f,0.0f},
	{0.0f,14.0f,0.0f},
	{2.0f,16.0f,0.0f},
	{14.0f,16.0f,0.0f},
	{16.0f,14.0f,0.0f},
	{16.0f,10.0f,0.0f},
	{14.0f,8.0f,0.0f},
	{10.0f,8.0f,1.0f}};

GameModel g_gmodel={sizeof(g_gpoints)/sizeof(g_gpoints[0]),g_gpoints};

GameVector3 g_hpoints[]={
	{0.0f,0.0f,0.0f},
	{0.0f,16.0f,1.0f},
	{16.0f,0.0f,0.0f},
	{16.0f,16.0f,1.0f},
	{0.0f,8.0f,0.0f},
	{16.0f,8.0f,1.0f}};

GameModel g_hmodel={sizeof(g_hpoints)/sizeof(g_hpoints[0]),g_hpoints};

GameVector3 g_ipoints[]={
	{8.0f,0.0f,0.0f},
	{8.0f,16.0f,1.0f}};

GameModel g_imodel={sizeof(g_ipoints)/sizeof(g_ipoints[0]),g_ipoints};

GameVector3 g_jpoints[]={
	{16.0f,0.0f,0.0f},
	{16.0f,14.0f,0.0f},
	{14.0f,16.0f,0.0f},
	{2.0f,16.0f,0.0f},
	{0.0f,14.0f,1.0f}};

GameModel g_jmodel={sizeof(g_jpoints)/sizeof(g_jpoints[0]),g_jpoints};

GameVector3 g_kpoints[]={
	{0.0f,0.0f,0.0f},
	{0.0f,16.0f,1.0f},
	{16.0f,0.0f,0.0f},
	{0.0f,8.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_kmodel={sizeof(g_kpoints)/sizeof(g_kpoints[0]),g_kpoints};

GameVector3 g_lpoints[]={
	{0.0f,0.0f,0.0f},
	{0.0f,16.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_lmodel={sizeof(g_lpoints)/sizeof(g_lpoints[0]),g_lpoints};

GameVector3 g_mpoints[]={
	{0.0f,16.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{16.0f,0.0f,0.0f},
	{16.0f,16.0f,1.0f},
	{8.0f,0.0f,0.0f},
	{8.0f,16.0f,1.0f}};

GameModel g_mmodel={sizeof(g_mpoints)/sizeof(g_mpoints[0]),g_mpoints};

GameVector3 g_npoints[]={
	{0.0f,16.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{16.0f,0.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_nmodel={sizeof(g_npoints)/sizeof(g_npoints[0]),g_npoints};

GameVector3 g_opoints[]={
	{0.0f,2.0f,0.0f},
	{2.0f,0.0f,0.0f},
	{14.0f,0.0f,0.0f},
	{16.0f,2.0f,0.0f},
	{16.0f,14.0f,0.0f},
	{14.0f,16.0f,0.0f},
	{2.0f,16.0f,0.0f},
	{0.0f,14.0f,0.0f}};

GameModel g_omodel={sizeof(g_opoints)/sizeof(g_opoints[0]),g_opoints};

GameVector3 g_ppoints[]={
	{0.0f,16.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{14.0f,0.0f,0.0f},
	{16.0f,2.0f,0.0f},
	{16.0f,6.0f,0.0f},
	{14.0f,8.0f,0.0f},
	{0.0f,8.0f,1.0f}};

GameModel g_pmodel={sizeof(g_ppoints)/sizeof(g_ppoints[0]),g_ppoints};

GameVector3 g_qpoints[]={
	{0.0f,2.0f,0.0f},
	{2.0f,0.0f,0.0f},
	{14.0f,0.0f,0.0f},
	{16.0f,2.0f,0.0f},
	{16.0f,14.0f,0.0f},
	{14.0f,16.0f,0.0f},
	{2.0f,16.0f,0.0f},
	{0.0f,14.0f,0.0f},
	{0.0f,2.0f,1.0f},
	{12.0f,12.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_qmodel={sizeof(g_qpoints)/sizeof(g_qpoints[0]),g_qpoints};

GameVector3 g_rpoints[]={
	{0.0f,16.0f,0.0f},
	{0.0f,0.0f,0.0f},
	{14.0f,0.0f,0.0f},
	{16.0f,2.0f,0.0f},
	{16.0f,6.0f,0.0f},
	{14.0f,8.0f,0.0f},
	{0.0f,8.0f,1.0f},
	{13.0f,8.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_rmodel={sizeof(g_rpoints)/sizeof(g_rpoints[0]),g_rpoints};

GameVector3 g_spoints[]={
	{16.0f,0.0f,0.0f},
	{2.0f,0.0f,0.0f},
	{0.0f,2.0f,0.0f},
	{0.0f,6.0f,0.0f},
	{2.0f,8.0f,0.0f},
	{14.0f,8.0f,0.0f},
	{16.0f,10.0f,0.0f},
	{16.0f,14.0f,0.0f},
	{14.0f,16.0f,0.0f},
	{0.0f,16.0f,1.0f}};

GameModel g_smodel={sizeof(g_spoints)/sizeof(g_spoints[0]),g_spoints};

GameVector3 g_tpoints[]={
	{0.0f,0.0f,0.0f},
	{16.0f,0.0f,1.0f},
	{8.0f,0.0f,0.0f},
	{8.0f,16.0f,1.0f}};

GameModel g_tmodel={sizeof(g_tpoints)/sizeof(g_tpoints[0]),g_tpoints};

GameVector3 g_upoints[]={
	{0.0f,0.0f,0.0f},
	{0.0f,16.0f,0.0f},
	{16.0f,16.0f,0.0f},
	{16.0f,0.0f,1.0f}};

GameModel g_umodel={sizeof(g_upoints)/sizeof(g_upoints[0]),g_upoints};

GameVector3 g_vpoints[]={
	{0.0f,0.0f,0.0f},
	{8.0f,16.0f,0.0f},
	{16.0f,0.0f,1.0f}};

GameModel g_vmodel={sizeof(g_vpoints)/sizeof(g_vpoints[0]),g_vpoints};

GameVector3 g_wpoints[]={
	{0.0f,0.0f,0.0f},
	{0.0f,16.0f,0.0f},
	{8.0f,10.0f,0.0f},
	{16.0f,16.0f,0.0f},
	{16.0f,0.0f,1.0f}};

GameModel g_wmodel={sizeof(g_wpoints)/sizeof(g_wpoints[0]),g_wpoints};

GameVector3 g_xpoints[]={
	{0.0f,0.0f,0.0f},
	{16.0f,16.0f,1.0f},
	{16.0f,0.0f,0.0f},
	{0.0f,16.0f,1.0f}};

GameModel g_xmodel={sizeof(g_xpoints)/sizeof(g_xpoints[0]),g_xpoints};

GameVector3 g_ypoints[]={
	{0.0f,0.0f,0.0f},
	{8.0f,8.0f,0.0f},
	{16.0f,0.0f,1.0f},
	{8.0f,8.0f,0.0f},
	{8.0f,16.0f,1.0f}};

GameModel g_ymodel={sizeof(g_ypoints)/sizeof(g_ypoints[0]),g_ypoints};

GameVector3 g_zpoints[]={
	{0.0f,0.0f,0.0f},
	{16.0f,0.0f,1.0f},
	{0.0f,16.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_zmodel={sizeof(g_zpoints)/sizeof(g_zpoints[0]),g_zpoints};

GameVector3 g_octpoints[]={
	{5.333f,0.0f,0.0f},
	{5.333f,16.0f,1.0f},
	{10.666f,0.0f,0.0f},
	{10.666f,16.0f,1.0f},
	{0.0f,5.333f,0.0f},
	{16.0f,5.333f,1.0f},
	{0.0f,10.666f,0.0f},
	{16.0f,10.666f,1.0f}};

GameModel g_octmodel={sizeof(g_octpoints)/sizeof(g_octpoints[0]),g_octpoints};

GameVector3 g_colonpoints[]={
	{8.0f,4.0f,0.0f},
	{8.0f,6.0f,1.0f},
	{8.0, 10.0f,0.0f},
	{8.0f,12.0f,1.0f}};

GameModel g_colonmodel={sizeof(g_colonpoints)/sizeof(g_colonpoints[0]),g_colonpoints};

GameVector3 g_sqoutepoints[]={
	{8.0f,0.0f,0.0f},
	{8.0f,6.0f,1.0f}};

GameModel g_sqoutemodel={sizeof(g_sqoutepoints)/sizeof(g_sqoutepoints[0]),g_sqoutepoints};


GameVector3 g_underscorepoints[]={
	{0.0f,16.0f,0.0f},
	{16.0f,16.0f,1.0f}};

GameModel g_underscoremodel={sizeof(g_underscorepoints)/sizeof(g_underscorepoints[0]),g_underscorepoints};

GameVector3 g_dashpoints[]={
	{0.0f,8.0f,0.0f},
	{16.0f,8.0f,1.0f}};

GameModel g_dashmodel={sizeof(g_dashpoints)/sizeof(g_dashpoints[0]),g_dashpoints};

void Ship::Collide(GameObject *o)
{
	if(m_state==GameBase::STATE_PLAY)
	{
		if(o->m_type==OBJTYPE_SHOT)
		{
			Shot *s=static_cast<Shot *>(o);

			/* did we shoot ourselves?? */
			if(s->m_ship==this)
				return;

			/* a shot from another ship is not fatal in COOP mode but is fatal in VS mode */
			/* todo: put in check for mode */
			return;
		}

		m_game->Explode(this,m_color);
		m_state=GameBase::STATE_DIE;
		m_statedelay=0;
		--m_life;
	}
}

void Ufo::Collide(GameObject *o)
{
	if(o->m_type==OBJTYPE_SHOT)
	{
		Shot *s=static_cast<Shot *>(o);

		s->m_ship->m_score+=1000;

		s->m_game->Explode(this,DrawColor(255,255,255));
		/* explode */
		m_active=false;
	}
}

void Shot::Collide(GameObject *o)
{
	m_active=false;

	/* player shot hit a rock? (UFO SHOTS don't count ) */
	if(m_type==OBJTYPE_SHOT && o->m_type==OBJTYPE_ROCK)
	{
		Rock *r=static_cast<Rock *>(o);
		static unsigned int scores[NUMROCKSIZES]={100,200,300};

		m_ship->m_score+=scores[r->m_size];
	}
}

void Rock::Collide(GameObject *o)
{
	if(o->m_type==OBJTYPE_SHOT)
	{
		m_game->Explode(this,DrawColor(255,255,255));
		m_game->RockHit(this);
	}
}

GameBase::GameBase()
{
	unsigned int i;

	m_title=false;
	m_playing=false;
	m_paused=false;
	m_lives=3;		/* default number of lives */

	/* make array of models for font characters */
	for(i=0;i<MAXFONTCHARS;++i)
		m_fontmodel[i]=0;

	/* add specific models to the table */
	m_fontmodel[(unsigned int)'.']=&g_decmodel;
	m_fontmodel[(unsigned int)'0']=&g_0model;
	m_fontmodel[(unsigned int)'1']=&g_1model;
	m_fontmodel[(unsigned int)'2']=&g_2model;
	m_fontmodel[(unsigned int)'3']=&g_3model;
	m_fontmodel[(unsigned int)'4']=&g_4model;
	m_fontmodel[(unsigned int)'5']=&g_5model;
	m_fontmodel[(unsigned int)'6']=&g_6model;
	m_fontmodel[(unsigned int)'7']=&g_7model;
	m_fontmodel[(unsigned int)'8']=&g_8model;
	m_fontmodel[(unsigned int)'9']=&g_9model;
	m_fontmodel[(unsigned int)'#']=&g_octmodel;
	m_fontmodel[(unsigned int)':']=&g_colonmodel;
	m_fontmodel[(unsigned int)'_']=&g_underscoremodel;
	m_fontmodel[(unsigned int)'-']=&g_dashmodel;
	m_fontmodel[(unsigned int)'\'']=&g_sqoutemodel;



	m_fontmodel[(unsigned int)'a']=&g_amodel;
	m_fontmodel[(unsigned int)'A']=&g_amodel;
	m_fontmodel[(unsigned int)'b']=&g_bmodel;
	m_fontmodel[(unsigned int)'B']=&g_bmodel;
	m_fontmodel[(unsigned int)'c']=&g_cmodel;
	m_fontmodel[(unsigned int)'C']=&g_cmodel;
	m_fontmodel[(unsigned int)'d']=&g_dmodel;
	m_fontmodel[(unsigned int)'D']=&g_dmodel;
	m_fontmodel[(unsigned int)'e']=&g_emodel;
	m_fontmodel[(unsigned int)'E']=&g_emodel;
	m_fontmodel[(unsigned int)'f']=&g_fmodel;
	m_fontmodel[(unsigned int)'F']=&g_fmodel;
	m_fontmodel[(unsigned int)'g']=&g_gmodel;
	m_fontmodel[(unsigned int)'G']=&g_gmodel;
	m_fontmodel[(unsigned int)'h']=&g_hmodel;
	m_fontmodel[(unsigned int)'H']=&g_hmodel;
	m_fontmodel[(unsigned int)'i']=&g_imodel;
	m_fontmodel[(unsigned int)'I']=&g_imodel;
	m_fontmodel[(unsigned int)'j']=&g_jmodel;
	m_fontmodel[(unsigned int)'J']=&g_jmodel;
	m_fontmodel[(unsigned int)'k']=&g_kmodel;
	m_fontmodel[(unsigned int)'K']=&g_kmodel;
	m_fontmodel[(unsigned int)'l']=&g_lmodel;
	m_fontmodel[(unsigned int)'L']=&g_lmodel;
	m_fontmodel[(unsigned int)'m']=&g_mmodel;
	m_fontmodel[(unsigned int)'M']=&g_mmodel;
	m_fontmodel[(unsigned int)'n']=&g_nmodel;
	m_fontmodel[(unsigned int)'N']=&g_nmodel;
	m_fontmodel[(unsigned int)'o']=&g_omodel;
	m_fontmodel[(unsigned int)'O']=&g_omodel;
	m_fontmodel[(unsigned int)'p']=&g_pmodel;
	m_fontmodel[(unsigned int)'P']=&g_pmodel;
	m_fontmodel[(unsigned int)'q']=&g_qmodel;
	m_fontmodel[(unsigned int)'Q']=&g_qmodel;
	m_fontmodel[(unsigned int)'r']=&g_rmodel;
	m_fontmodel[(unsigned int)'R']=&g_rmodel;
	m_fontmodel[(unsigned int)'s']=&g_smodel;
	m_fontmodel[(unsigned int)'S']=&g_smodel;
	m_fontmodel[(unsigned int)'t']=&g_tmodel;
	m_fontmodel[(unsigned int)'T']=&g_tmodel;
	m_fontmodel[(unsigned int)'u']=&g_umodel;
	m_fontmodel[(unsigned int)'U']=&g_umodel;
	m_fontmodel[(unsigned int)'v']=&g_vmodel;
	m_fontmodel[(unsigned int)'V']=&g_vmodel;
	m_fontmodel[(unsigned int)'w']=&g_wmodel;
	m_fontmodel[(unsigned int)'W']=&g_wmodel;
	m_fontmodel[(unsigned int)'x']=&g_xmodel;
	m_fontmodel[(unsigned int)'X']=&g_xmodel;
	m_fontmodel[(unsigned int)'y']=&g_ymodel;
	m_fontmodel[(unsigned int)'Y']=&g_ymodel;
	m_fontmodel[(unsigned int)'z']=&g_zmodel;
	m_fontmodel[(unsigned int)'Z']=&g_zmodel;
}

void GameBase::Init(int w,int h)
{
	m_w=w;
	m_h=h;
	SetScreenCenter(w/2.0f,h/2.0f);
}

GameBase::~GameBase()
{
}

void GameBase::StartTitle(void)
{
	m_title=true;
	m_level=10;
	m_numplayers=0;
	m_gameover=false;
	m_playing=false;
	StartLevel();
}

void GameBase::Start(void)
{
	unsigned int i;
	Ship *ship;

	m_title=false;
	m_numplayersleft=m_numplayers;
	m_level=0;
	m_gameover=false;
	m_playing=true;
	m_ufo.m_appearside=false;
	for(i=0;i<MAXPLAYERS;++i)
	{
		ship=&m_ship[i];
		ship->m_life=m_lives;
		ship->m_score=0;
		ship->m_color=m_shipcolors[i];
		ship->m_thrustsound=false;
	}

	StartLevel();
}

void GameBase::StartLevel(void)
{
	unsigned int i;
	unsigned int j;
	Ship *ship;
	Shot *shot;
	Rock *rock;
	GameObject **go=m_objects;	/* collision objects list */
	GameParticle *particle;

	m_paused=false;

	/* init the players first */
	for(i=0;i<MAXPLAYERS;++i)
	{
		ship=&m_ship[i];
		*(go++)=ship;
		ship->m_type=OBJTYPE_SHIP;
		ship->m_index=i;
		ship->m_shotdelay=0;
		ship->m_game=this;
		ship->m_state=STATE_APPEAR;
		ship->m_statedelay=0;

		if(!m_level)
		{
			ship->m_active=(i<m_numplayers);
			ship->m_heading=0.0f;
			ship->SetModel(&g_shipmodel);

			ship->m_x=((m_numplayers/2.0f)*-60.0f)+(i*60.0f);
			ship->m_y=0.0f;
			ship->m_speedx=0.0f;
			ship->m_speedy=0.0f;
			ship->m_showthrust=0;
		}
		for(j=0;j<MAXSHOTS;++j)
		{
			shot=&ship->m_shot[j];
			*(go++)=shot;

			shot->m_type=OBJTYPE_SHOT;
			shot->m_ship=ship;
			shot->m_game=this;
			shot->m_active=false;

			shot->SetModel(g_shotmodel);
		}
	}

	*(go++)=&m_ufo;
	m_ufo.m_type=OBJTYPE_UFO;
	m_ufo.SetModel(&g_ufomodel);
	m_ufo.m_game=this;
	m_ufo.m_active=true;
	m_ufo.m_heading=0.0f;
	m_ufo.m_x=0.0f;
	m_ufo.m_y=-100.0f;
	m_ufo.m_active=false;
	m_ufo.m_appeardelay=0;
	m_ufo.m_shotdelay=0;

	for(j=0;j<MAXSHOTS;++j)
	{
		shot=&m_ufo.m_shot[j];
		*(go++)=shot;

		shot->m_type=OBJTYPE_UFOSHOT;
		shot->m_ship=0;
		shot->m_game=this;
		shot->m_active=false;

		shot->SetModel(g_shotmodel);
	}

	m_beat=0;
	m_beatcount=0;
	m_beatdelay=60-(m_level*3);

	/* vary the number of rocks depending on the level */

	m_numrocks=4+m_level;
	if(m_numrocks>12)
		m_numrocks=12;

	/* init the rocks */
	for(i=0;i<MAXROCKS;++i)
	{
		rock=&m_rock[i];
		*(go++)=rock;
		rock->m_game=this;
		rock->m_type=OBJTYPE_ROCK;

		if(i<m_numrocks)
		{
			rock->m_active=true;
			rock->m_size=0;
			rock->m_x=Rand(m_w)-m_cx;
			rock->m_y=Rand(m_h)-m_cy;
			GenerateRock(rock);
		}
		else
			rock->m_active=false;
	}

	/* initialize the used particles linked list to empty */
	m_usedphead.m_prev=0;
	m_usedphead.m_next=&m_usedptail;
	m_usedptail.m_prev=&m_usedphead;
	m_usedptail.m_next=0;

	/* initialize the free particles linked list to all particles */
	m_freephead.m_prev=0;
	particle=&m_freephead;
	for(i=0;i<MAXPARTICLES;++i)
	{
		particle->m_next=&m_particle[i];
		m_particle[i].m_prev=particle;
		particle=&m_particle[i];
	}
	particle->m_next=&m_freeptail;
	m_freeptail.m_prev=particle;
	m_freeptail.m_next=0;
}

void GameBase::GenerateRock(Rock *rock)
{
	int shape;
	static double movespeed[NUMROCKSIZES]={1.5f,2.5f,3.5f};
	double ms=movespeed[rock->m_size];

	rock->m_heading=Rand(0.0f,1.0f);
	rock->m_headingspeed=Rand(-0.01f,0.01f);
	rock->m_speedx=Rand(-ms,ms);
	rock->m_speedy=Rand(-ms,ms);

	shape=Rand(NUMROCKMODELS);
	rock->SetModel(&g_rockmodels[(rock->m_size*NUMROCKMODELS)+shape]);
}

void GameBase::Draw(unsigned int w,unsigned int h)
{
	unsigned int i;
	unsigned int j;
	Ship *ship;
	Shot *shot;
	Rock *rock;
	double scorex,scorey;
	char tempstring[64];;
	GameParticle *gp;

	m_w=w;
	m_h=h;
	SetScreenCenter(w/2.0f,h/2.0f);

	/* draw ships */
	if(m_title==false)
	{
		scorex=(-m_cx)+3.0f;
		scorey=(-m_cy)+3.0f;
		DrawString("SCORE",1.0f,scorex,scorey,0xffffffff);
		scorex+=StringPix("Score",1.0f)+32.0f;

		for(i=0;i<m_numplayers;++i)
		{
			ship=&m_ship[i];

			/* show lives */
			for(j=0;j<ship->m_life;++j)
			{
				DrawModel(scorex,scorey+12.0f,0.0f,ship->m_color,ship->m_model,0.40f,0.0f);
				scorex+=14.0f;
			}

			/* draw the score */
			sprintf(tempstring,"%08d",ship->m_score);
			DrawString(tempstring,1.0f,scorex,scorey,ship->m_color);
			scorex+=StringPix(tempstring,1.0f);

			if(ship->m_active)
			{
				if(ship->m_state==STATE_DIE)
				{
					/* show 4 sides breaking apart */
					double h=(ship->m_heading-0.25f)*(PI*2);
					for(j=0;j<4;++j)
					{
						double x=ship->m_x-sin(h)*ship->m_statedelay;
						double y=ship->m_y-cos(h)*ship->m_statedelay;;
						DrawModel(x,y,ship->m_heading,ship->m_color,&g_exshipmodels[j],1.0f,0.0f);
						h+=PI/2.0f;
					}
				}
				else if(ship->m_draw)
				{
					DrawModel(ship->m_x,ship->m_y,ship->m_heading,ship->m_color,ship->m_model,1.0f,0.0f);
					if(ship->m_showthrust)
						DrawModel(ship->m_x,ship->m_y,ship->m_heading,ship->m_color,&g_thrustmodel[ship->m_showthrust-1],1.0f,0.0f);
				}
			}
			for(j=0;j<MAXSHOTS;++j)
			{
				shot=&ship->m_shot[j];
				if(shot->m_active)
					DrawModel(shot->m_x,shot->m_y,shot->m_heading,ship->m_color,shot->m_model,1.0f,0.0f);
			}
		}

		/* draw ufo */
		if(m_ufo.m_active)
			DrawModel(m_ufo.m_x,m_ufo.m_y,0.0f,DrawColor(255,255,255),m_ufo.m_model,1.0f,0.0f);
		for(j=0;j<MAXSHOTS;++j)
		{
			shot=&m_ufo.m_shot[j];
			if(shot->m_active)
				DrawModel(shot->m_x,shot->m_y,shot->m_heading,DrawColor(255,255,255),shot->m_model,1.0f,0.0f);
		}

		/* draw particles */
		gp=m_usedphead.m_next;
		while(gp!=&m_usedptail)
		{
			DrawRect(gp->m_x,gp->m_y,gp->m_x+1,gp->m_y+1,gp->m_color);
			gp=gp->m_next;
		}
	}

	/* draw rocks */
	for(i=0;i<MAXROCKS;++i)
	{
		rock=&m_rock[i];
		if(rock->m_active)
			DrawModel(rock->m_x,rock->m_y,rock->m_heading,DrawColor(255,255,255),rock->m_model,1.0f,0.5f);
	}
}

void GameBase::CheckWrap(double *px,double *py)
{
	double x;
	double y;

	if(px)
	{
		x=px[0];
		if(x<-(m_cx))
			x+=m_cx*2.0f;
		else if(x>m_cx)
			x-=m_cx*2.0f;
		px[0]=x;
	}

	if(py)
	{
		y=py[0];
		if(y<-(m_cy))
			y+=m_cy*2.0f;
		else if(y>m_cy)
			y-=m_cy*2.0f;
		py[0]=y;
	}
}

void GameBase::Update(void)
{
	unsigned int i;
	unsigned int j;
	Ship *ship;
	Shot *shot;
	Rock *rock;
	GameObject **po1;
	GameObject **po2;
	GameObject *o1;
	GameObject *o2;
	double dist;
	bool hit;
	double dx,dy,maxsum;
	bool pauserequest=false;
	unsigned int pauseuser=0;
	bool thrust;

	if(m_paused || m_gameover)
		return;

	if(m_title==false)
	{
		if(++m_beatcount==m_beatdelay)
		{
			m_beatcount=0;
			PlaySound(SOUND_BEAT1+m_beat);
			m_beat^=1;
		}

		/* update ships */
		for(i=0;i<m_numplayers;++i)
		{
			ship=&m_ship[i];
			if(IsPressed(i,GAMEINPUT_PAUSE))
			{
				pauserequest=true;
				pauseuser=i;
			}
			thrust=false;
			if(ship->m_active)
			{
				switch(ship->m_state)
				{
				case STATE_APPEAR:
					/* flash so they know they are in 'invincible mode' */
					ship->m_draw=((ship->m_statedelay&2)!=0);
					if(++(ship->m_statedelay)==60)
					{
						ship->m_state=STATE_PLAY;
						ship->m_draw=true;
					}
					/* fall through */
				case STATE_PLAY:
					if(IsPressed(i,GAMEINPUT_LEFT))
						ship->m_heading-=3.0f/360.0f;
					else if(IsPressed(i,GAMEINPUT_RIGHT))
						ship->m_heading+=3.0f/360.0f;

					if(IsPressed(i,GAMEINPUT_THRUST))
					{
						GameMatrix33 mat;
						GameVector3 vin;
						GameVector3 vout;
						double newspeedx,newspeedy,newspeed,scale;

						thrust=true;
						/* add to speed vector based on current heading */
						mat.SetRotZ(ship->m_heading);
						vin.m_x=0.0f;
						vin.m_y=-0.5f;
						vin.m_z=0.0f;
						mat.Transform(&vin,&vout);
						newspeedx=ship->m_speedx+vout.m_x;
						newspeedy=ship->m_speedy+vout.m_y;
						newspeed=hypot(newspeedx,newspeedy);
						if(newspeed>MAXSPEED)
						{
							/* scale */
							scale=MAXSPEED/newspeed;
							newspeedx*=scale;
							newspeedy*=scale;
						}
						ship->m_speedx=newspeedx;
						ship->m_speedy=newspeedy;

						if(++(ship->m_showthrust)>=4)
							ship->m_showthrust=0;
					}
					else
					{
						/* decel */
						ship->m_speedx*=0.975f;
						ship->m_speedy*=0.975f;
						ship->m_showthrust=0;
					}
					ship->m_x+=ship->m_speedx;
					ship->m_y+=ship->m_speedy;
					CheckWrap(&ship->m_x,&ship->m_y);

					if(ship->m_shotdelay)
						--ship->m_shotdelay;
					else if(IsPressed(i,GAMEINPUT_FIRE))
					{
						shot=0;
						/* fire a shot if we have an available shot slot */
						for(j=0;j<MAXSHOTS;++j)
						{
							if(ship->m_shot[j].m_active==false)
							{
								shot=&ship->m_shot[j];
								break;
							}
						}
						if(shot)
						{
							GameMatrix33 mat;
							GameVector3 vin;
							GameVector3 vout;

							PlaySound(SOUND_SHOT);
							ship->m_shotdelay=SHOTDELAY;
							shot->m_active=true;
							/* calc position of shot and direction */
							mat.SetRotZ(ship->m_heading);
							vin.m_x=0.0f;
							vin.m_y=-10.0f;
							vin.m_z=0.0f;
							mat.Transform(&vin,&vout);
							shot->m_x=ship->m_x+vout.m_x;
							shot->m_y=ship->m_y+vout.m_y;
							shot->m_heading=ship->m_heading;
							shot->m_speedx=vout.m_x;
							shot->m_speedy=vout.m_y;
							shot->m_life=0;
						}
					}
				break;
				case STATE_DIE:
					++(ship->m_statedelay);
					switch(ship->m_statedelay)
					{
					case 20:
					break;
					case 40:
					break;
					case 60:
						if(ship->m_life)
						{
							ship->m_state=STATE_APPEAR;
							ship->m_statedelay=0;
							ship->m_x=0.0f;
							ship->m_y=0.0f;
							ship->m_heading=0.0f;
							ship->m_speedx=0.0f;
							ship->m_speedy=0.0f;
						}
						else
						{
							ship->m_active=false;
							--m_numplayersleft;
							if(!m_numplayersleft)
							{
								/* game over */
								GameOver();
								m_gameover=true;
							}
						}
					break;
					}
				}
			}
			/* move all shots */
			for(j=0;j<MAXSHOTS;++j)
			{
				shot=&ship->m_shot[j];
				if(shot->m_active)
				{
					shot->m_x+=shot->m_speedx;
					shot->m_y+=shot->m_speedy;
					CheckWrap(&shot->m_x,&shot->m_y);
					if(++shot->m_life==SHOTLIFE)
						shot->m_active=false;
				}
			}
			if(ship->m_thrustsound==false && thrust==true)
			{
				ship->m_thrustsound=true;
				ship->m_thrusthandle=StartSound(SOUND_THRUST);
			}
			else if(ship->m_thrustsound==true && thrust==false)
			{
				ship->m_thrustsound=false;
				StopSound(ship->m_thrusthandle);
			}
		}

		/* UFO update */
		if(m_ufo.m_active)
		{
			if(!m_ufo.m_segmentlen)
			{
				/* randonly move straight or on a 45 up or down */
				m_ufo.m_segmentlen=Rand((int)(m_cy*0.5f))+(int)(m_cy*0.25f);
				switch(Rand(9))
				{
				case 0:
				case 1:
					m_ufo.m_curheading=m_ufo.m_heading-0.125f;
				break;
				case 2:
				case 3:
					m_ufo.m_curheading=m_ufo.m_heading+0.125f;
				break;
				default:
					m_ufo.m_curheading=m_ufo.m_heading;
				break;
				}
			}
			else
				--(m_ufo.m_segmentlen);

			m_ufo.m_x+=cos(m_ufo.m_curheading*(2.0f*PI))*3.0f;
			m_ufo.m_y+=sin(m_ufo.m_curheading*(2.0f*PI))*3.0f;
			CheckWrap(0,&m_ufo.m_y);

			/* done? */
			if(fabs(m_ufo.m_x)>=(m_cx+m_ufo.m_maxradius))
			{
				m_ufo.m_active=false;
			}
			else
			{
				/* time to fire any new shots? */
				if(++(m_ufo.m_shotdelay)==(30))
				{
					m_ufo.m_shotdelay=0;

					shot=0;
					/* fire a shot if we have an available shot slot */
					for(j=0;j<MAXSHOTS;++j)
					{
						if(m_ufo.m_shot[j].m_active==false)
						{
							shot=&m_ufo.m_shot[j];
							break;
						}
					}
					if(shot)
					{
						unsigned int n;
						double h;
						double err;
						GameMatrix33 mat;
						GameVector3 delta;
						GameVector3 vin;
						GameVector3 vout;

						PlaySound(SOUND_SHOT);

						/* calc vector between UFO and a random SHIP */
						n=Rand(m_numplayers);
						delta.m_x=m_ufo.m_x-m_ship[n].m_x;
						delta.m_y=m_ufo.m_y-m_ship[n].m_y;
						h=(atan2(delta.m_y,delta.m_x)*(1.0f/(2.0f*PI)))-0.25f;

						/* add more randomness for lower levels */
						err=(6-m_level)*(0.10f/6.0f);
						if(err>0.0f)
						{
							err=Rand(-err,err);
							h+=err;
						}

						shot->m_active=true;
						shot->m_heading=h;

						/* calc position of shot and direction */
						mat.SetRotZ(h);
						vin.m_x=0.0f;
						vin.m_y=-10.0f;
						vin.m_z=0.0f;
						mat.Transform(&vin,&vout);
						shot->m_x=m_ufo.m_x+vout.m_x;
						shot->m_y=m_ufo.m_y+vout.m_y;
						shot->m_speedx=vout.m_x;
						shot->m_speedy=vout.m_y;
						shot->m_life=0;
					}
				}
			}

		}
		else
		{
			/* make delay vary based on level?? */
			if(++m_ufo.m_appeardelay==(20*60))
			{
				m_ufo.m_active=true;
				m_ufo.m_appeardelay=0;
				m_ufo.m_appearside=!m_ufo.m_appearside;
				if(m_ufo.m_appearside==false)
				{
					m_ufo.m_x=-(m_cx+m_ufo.m_maxradius);
					m_ufo.m_heading=0.0f;	/* left to right */
				}
				else
				{
					m_ufo.m_x=(m_cx+m_ufo.m_maxradius);
					m_ufo.m_heading=0.5f;	/* right to left */
				}
				m_ufo.m_y=Rand(-m_cy*0.9f,m_cy*0.9f);
				m_ufo.m_segmentlen=0;
			}
		}

		/* move all ufo shots */
		for(j=0;j<MAXSHOTS;++j)
		{
			shot=&m_ufo.m_shot[j];
			if(shot->m_active)
			{
				shot->m_x+=shot->m_speedx;
				shot->m_y+=shot->m_speedy;
				CheckWrap(&shot->m_x,&shot->m_y);
				if(++shot->m_life==SHOTLIFE)
					shot->m_active=false;
			}
		}

		/* update all used particles */
		{
			GameParticle *gp;
			GameParticle *pp;
			GameParticle *np;
			GameParticle *sp;

			gp=m_usedphead.m_next;
			while(gp!=&m_usedptail)
			{
				--gp->m_life;
				if(!gp->m_life)
				{
					/* save pointer to next particle */
					sp=gp->m_next;
					/* done, remove from used list */
					pp=gp->m_prev;
					np=gp->m_next;
					pp->m_next=np;
					np->m_prev=pp;
					/* add me to the free list */
					pp=&m_freephead;
					np=pp->m_next;
					pp->m_next=gp;
					gp->m_prev=pp;
					gp->m_next=np;
					np->m_prev=gp;
					/* continue with next particle */
					gp=sp;
				}
				else
				{
					gp->m_x+=gp->m_speedx;
					gp->m_y+=gp->m_speedy;
					gp=gp->m_next;
				}
			}
		}

	}

	/* update rocks */
	for(i=0;i<MAXROCKS;++i)
	{
		rock=&m_rock[i];
		if(rock->m_active)
		{
			rock->m_heading+=rock->m_headingspeed;
			rock->m_x+=rock->m_speedx;
			rock->m_y+=rock->m_speedy;
			CheckWrap(&rock->m_x,&rock->m_y);
		}
	}

	/* check for collisions */
	po1=m_objects;
	for(i=0;i<TOTALOBJECTS-1;++i,++po1)
	{
		o1=*(po1);
		if(o1->m_active)
		{
			po2=po1+1;
			for(j=i+1;j<TOTALOBJECTS;++j,++po2)
			{
				o2=*(po2);
				if(o2->m_active)
				{
					if(o1->m_type!=o2->m_type)
					{
						maxsum=o1->m_maxradius+o2->m_maxradius;
						/* do a quick exclude without hypot */
						dx=o1->m_x-o2->m_x;
						if(fabs(dx)>maxsum)
							continue;
						dy=o1->m_y-o2->m_y;
						if(fabs(dy)>maxsum)
							continue;

						/* two active objects, check distance between them */
						dist=hypot(dx,dy);
						if(dist<(maxsum))
						{
							if(dist<(o1->m_minradius+o2->m_minradius))
								hit=true;
							else
							{
								/* possible hit */
								hit=IsInside(o1,o2);
								if(!hit)
									hit=IsInside(o2,o1);
							}

							if(hit)
							{
								/* yes, objects collided */
								o1->Collide(o2);
								o2->Collide(o1);
								if(o1->m_active==false)
									goto skiprest;
							}
						}
					}
				}
			}
		}
skiprest:;
	}

	/* call virtual function too let them know that the game is now paused */
	if(pauserequest)
		PauseRequest(pauseuser);
}

GameParticle * GameBase::AddParticle(double x,double y,double heading,double speed)
{
	GameParticle *gp;
	GameParticle *np;

	gp=m_freephead.m_next;
	if(gp==&m_freeptail)
		return(0);					/* no free particles! */

	/* remove me from free list */
	np=gp->m_next;
	m_freephead.m_next=np;
	np->m_prev=&m_freephead;

	/* add me to the used list */
	np=m_usedphead.m_next;
	m_usedphead.m_next=gp;
	gp->m_prev=&m_usedphead;
	gp->m_next=np;
	np->m_prev=gp;

	gp->m_x=x;
	gp->m_y=y;
	gp->m_speedx=sin(heading*(2*PI))*speed;
	gp->m_speedy=cos(heading*(2*PI))*speed;
	gp->m_color=DrawColor(255,255,255);		/* defaul to white */
	gp->m_life=15;
	return(gp);
}

bool GameBase::IsInside(GameObject *o1,GameObject *o2)
{
	double px;
	double py;
	int onvert;
	int nvert;
	GameVector3 *spoints;
	int	a,i,j=0;
	bool odd;
	GameMatrix33 mat;
	GameVector3 opoint[64];
	GameVector3 point[64];

	/* transform objects1 points */
	onvert=o1->m_collpoints;
	spoints=o1->m_model->m_points;
	mat.SetRotZ(o1->m_heading);
	mat.SetTX(o1->m_x);
	mat.SetTY(o1->m_y);
	for(i=0;i<onvert;++i)
		mat.Transform(spoints+i,opoint+i);

	nvert=o2->m_collpoints;
	spoints=o2->m_model->m_points;
	mat.SetRotZ(o2->m_heading);
	mat.SetTX(o2->m_x);
	mat.SetTY(o2->m_y);
	for(i=0;i<nvert;++i)
		mat.Transform(spoints+i,point+i);

	/* check each point in object1 to see if it is inside of object2 */
	for(a=0;a<onvert;++a)
	{
		px=opoint[a].m_x;
		py=opoint[a].m_y;

		odd=false;
		for (i=0; i<nvert; i++)
		{
			j++;
			if (j==nvert)
				j=0;
			if (point[i].m_y<py && point[j].m_y>=py ||  point[j].m_y<py && point[i].m_y>=py)
			{
				if (point[i].m_x+(py-point[i].m_y)/(point[j].m_y-point[i].m_y)*(point[j].m_x-point[i].m_x)<px)
					odd=!odd;
			}
		}
		if(odd)
			return(true);
	}
	return (false);
}

void GameBase::RockHit(Rock *rock)
{
	unsigned int i;
	unsigned int size;
	double radius;
	Rock *newrock;

	if(++(rock->m_size)==NUMROCKSIZES)
	{
		/* smallest size! */
		rock->m_active=false;
		--m_numrocks;
		if(!m_numrocks)
		{
			++m_level;
			StartLevel();
		}
	}
	else
	{
		/* make this one a smaller rock */
		GenerateRock(rock);
		/* make a few more too */
		size=rock->m_size;
		radius=rock->m_maxradius*0.75f;
		for(i=0;i<MAXROCKS;++i)
		{
			newrock=&m_rock[i];
			if(newrock->m_active==false)
			{
				newrock->m_active=true;

				newrock->m_x=rock->m_x+Rand(-radius,radius);
				newrock->m_y=rock->m_y+Rand(-radius,radius);
				newrock->m_size=size;
				GenerateRock(newrock);
				m_numrocks++;
				return;
			}
		}
	}
}

void GameBase::Explode(GameObject *obj,GameColor color)
{
	unsigned int i;
	GameParticle *gp;

	PlaySound(SOUND_EXPLODE);

	for(i=0;i<32;++i)
	{
		gp=AddParticle(obj->m_x,obj->m_y,Rand(0.0f,1.0f),Rand(2.0f,6.0f));
		if(!gp)
			return;
		gp->m_color=color;
	}
}

double GameBase::StringPix(const char *string,double scale)
{
	double pix=0.0f;
	double size=16.0f*scale;
	double gap=5.0f*scale;

	while(string[0])
	{
		if(string[0]==' ')
			pix+=(gap+gap);
		else
			pix+=(size+gap);
		++string;
	}
	return(pix);
}

void GameBase::DrawStringC(const char *string,double scale,double x,double y,GameColor color)
{
	x-=StringPix(string,scale)/2.0f;
	DrawString(string,scale,x,y,color);
}

void GameBase::DrawString(const char *string,double scale,double x,double y,GameColor color)
{
	double size=16.0f*scale;
	double gap=5.0f*scale;

	GameModel *model;

	while(string[0])
	{
		if(string[0]==' ')
			x+=(gap+gap);
		else
		{
			model=m_fontmodel[(unsigned int)string[0]];
			if(model)
				DrawModel(x,y,0.0f,color,model,scale,scale);
			else
				DrawRect(x,y,x+size,y+size,color);
			x+=(size+gap);
		}
		++string;
	}
}

/* calc largest and smallest radius for collision detection */
void GameObject::SetModel(GameModel *model)
{
	int i;
	double r,minr,maxr;
	int n;
	GameVector3 *p;

	m_model=model;
	n=model->m_num;
	p=model->m_points;

	/* collision points are only the outline, not inside points */
	for(i=0;i<n;++i)
	{
		if(p->m_z)
			break;
		p++;
	}
	m_collpoints=i;

	p=model->m_points;
	minr=maxr=hypot(p->m_x,p->m_y);
	++p;
	for(i=1;i<n;++i)
	{
		r=hypot(p->m_x,p->m_y);
		if(r>maxr)
			maxr=r;
		if(r<minr)
			minr=r;
		++p;
	}
	m_minradius=minr;
	m_maxradius=maxr;
}

GameModel *GameBase::GetShipModel(void)
{
	return &g_shipmodel;
}

