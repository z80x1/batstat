#include <bits/stdc++.h>
#include <unistd.h>
#include <curses.h>
#include <string>
#include <dirent.h>

#define PATH "/sys/class/power_supply/"
#define LOG_P 20
#define MILLION 1000000
using namespace std;

int refreshRate = 3;
//const string logFile = "log.txt";

float maxCharge, currentCharge, initCharge; // Ah*1E-6
float currentPower; // W
float currentVoltage; // V * 1E-6
float volt_min; // /sys/class/power_supply/BAT0/voltage_min_design in Volts
int initTime, percent, timeNow, timeElapsed = -refreshRate;
int lastTime = 0, logIndex = 0;
string Path = PATH, status;
bool quit = false;
WINDOW * mainwin;
vector <string> logCache;

void init()
{
	DIR* dir = opendir("/sys/class/power_supply/BAT0");
	if (dir)
	{
	   Path += "BAT0/";
	}
	else Path += "BAT1/";

  if ( (mainwin = initscr()) == NULL ) {
	  fprintf(stderr, "Error initialising ncurses.\n");
	  exit(EXIT_FAILURE);
  }
  keypad(stdscr, TRUE);
	
	if(has_colors() == FALSE)
	{	
		printf("Your terminal does not support color\n");
	}
	start_color();

  assume_default_colors(COLOR_WHITE,COLOR_BLACK);

	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_RED, COLOR_BLACK);
	init_pair(3, COLOR_GREEN, COLOR_BLACK);

	bkgd(COLOR_PAIR(1));

	ifstream chNow(Path+"charge_now");
	chNow >> initCharge;

	ifstream ifVoltMin(Path+"voltage_min_design");
    ifVoltMin >> volt_min;
    volt_min /= MILLION;
	
	initTime = time(NULL);
	
	ifstream maxChargeFile(Path+"charge_full");
	maxChargeFile >> maxCharge;
}

void refreshValues()
{
    float I;
    ifstream volNow(Path+"voltage_now");
	ifstream curNow(Path+"current_now");
	ifstream chNow(Path+"charge_now");
	ifstream st(Path+"status");
    
	curNow >> I;
	volNow >> currentVoltage;
    currentPower = I/MILLION * currentVoltage/MILLION;
	chNow >> currentCharge;
	timeNow = time(NULL);
	//timeElapsed = timeElapsed + timeNow - lastTime;
	timeElapsed += refreshRate;
	lastTime = timeNow;
	st >> status;
}

#if 0
void print()
{
	system("clear");
	printf("%-30s%s\n", "Status: ", status.c_str());
	printf("%-30s%.2lf Wh\n", "Max energy:", maxCharge/1000000*volt_min);
	printf("%-30s%.2lf Wh\n", "Energy left:", currentCharge/1000000*volt_min);
	printf("%-30s%.2lf W\n", "Power Consumption:", currentPower);
	printf("%-30s%.2lf\%\n", "Percentage left:", currentCharge/maxCharge*100);
	printf("%-30s%2d:%2d:%2d since %.2lf\%\n", "Time elapsed:", timeElapsed/3600, (timeElapsed/60)%60, timeElapsed%60, initCharge/maxCharge*100);
}
#endif

void newPrint()
{
	char buff[255];
	int line = 0;
    erase();
	sprintf(buff, "%-30s\n", "Status: ");
	mvaddstr(line, 0, buff);
	
	if(status[0] == 'C')
		attron(COLOR_PAIR(3));
	else
		attron(COLOR_PAIR(2));
	mvaddstr(line, strlen(buff) - 1, status.c_str());

	attron(COLOR_PAIR(1));
	sprintf(buff, "%-30s%.2lf Wh\n", "Max energy:", maxCharge/1000000*volt_min);
	mvaddstr(++line, 0, buff);
	sprintf(buff, "%-30s%.2lf Wh\n", "Energy left:", currentCharge/1000000*volt_min);
	mvaddstr(++line, 0, buff);
	sprintf(buff, "%-30s%.2lf V\n", "Voltage:", currentVoltage/MILLION);
	mvaddstr(++line, 0, buff);
	sprintf(buff, "%-30s%.2lf W\n", "Power Consumption:", currentPower);
	mvaddstr(++line, 0, buff);
	sprintf(buff, "%-30s%.2lf%%\n", "Percentage left:", currentCharge/maxCharge*100);
	mvaddstr(++line, 0, buff);
	sprintf(buff, "%-30s%.2lf W\n", "Average power Consumption:", (initCharge - currentCharge) / 1000000 / (1. * timeElapsed / 3600.));
	ofstream ferr("log.txt");
	ferr << initCharge - currentCharge << " Whr" << endl;
	ferr << (1. * timeElapsed / 3600.) << " s" << endl;
	mvaddstr(++line, 0, buff);
	sprintf(buff, "%-30s%2d:%2d:%2d since %.2lf%%\n", "Time elapsed:", timeElapsed/3600, (timeElapsed/60)%60, timeElapsed%60, initCharge/maxCharge*100);
	mvaddstr(++line, 0, buff);

	sprintf(buff, "= Time   ======== Percent ============================================\n");
	mvaddstr(++line, 0, buff);

    ++line;
	for(int i = logIndex; i < logCache.size(); i++) {
		mvaddstr(line + i - logIndex, 0, logCache[i].c_str());
    }

	refresh();
}

void addLog()
{
	char buff[200];
	sprintf(buff, "%2d:%2d:%2d          %.2lf\%\n", timeElapsed/3600, (timeElapsed/60)%60, timeElapsed%60, currentCharge/maxCharge*100);
	logCache.push_back(buff);
}

void keyListenerFunction()
{
	int ch = 0;
	while((ch = wgetch(mainwin)) != 'q')
	{
		if(ch == 27)
			break;

		switch ( ch ) {

    case KEY_UP:
        if ( logIndex > 0 )
        --logIndex;
      	newPrint();
        break;

    case KEY_DOWN:
        if ( logIndex + 1 < logCache.size() )
        ++logIndex;
    		newPrint();
        break;
    }
	}
	quit = 1;
	return;
}

void coreFunction()
{
	lastTime = time(NULL);
	// sleep(1);
	while(!quit)
	{
		refreshValues();
		newPrint();
		if(timeElapsed % LOG_P == 0)
			addLog();
		sleep(refreshRate);
	}
}

int main()
{
	
	init();
	thread keyListener(keyListenerFunction);
	thread core(coreFunction);
	thread *p;
	keyListener.join();
	//core.join();
	//terminate();
	//delete p;
	core.detach();
	erase();
  delwin(mainwin);
  endwin();
  return 0;
}
