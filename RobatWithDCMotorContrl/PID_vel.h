#ifndef PID_vel_h
#define PID_vel_h
#define LIBRARY_VERSION	1.1.1
#include <Arduino.h>
#include "motor_config.h"

class PID_vel
{
  //Constants used in some of the functions below
  #define AUTOMATIC	1
  #define MANUAL	0
  #define DIRECT  0
  #define REVERSE  1
  #define P_ON_M 0
  #define P_ON_E 1
public:
  //commonly used functions **************************************************************************
    PID_vel(double*, double*, double*,        // * constructor.  links the PID to the Input, Output, and 
        double, double, double, int, int);//   Setpoint.  Initial tuning parameters are also set here.
                                          //   (overload for specifying proportional mode)
    PID_vel(double*, double*, double*,        // * constructor.  links the PID to the Input, Output, and 
        double, double, double);//   Setpoint.  Initial tuning parameters are also set here.
                                          //   (overload for specifying proportional mode)

	
    void SetMode(int Mode);               // * sets PID to either Manual (0) or Auto (non-0)

    bool Compute();                       // * performs the PID calculation.  it should be
                                          //   called every time loop() cycles.
                                          //   
                                          // 

    void SetOutputLimits(double, double); // * 
										                      //   
    void SetTunings(double, double,       //  *
                    double);										                    
    void SetSampleTime(int);              // * sets the frequency, in Milliseconds, with which 
                                          //   the PID calculation is performed.  default is 100 ms

  //End**************************************************************************
  //Display functions ****************************************************************
	double GetKp();						  // These functions query the pid for interal values.
	double GetKi();						  //  they were created mainly for the pid front-end,
	double GetKd();						  // where it's important to know what is actually 
	int GetMode();						  //  inside the PID.//

  private:
	void Initialize();
	
	double dispKp;				// * we'll hold on to the tuning parameters in user-entered 
	double dispKi;				//   format for display purposes
	double dispKd;				//
      // output += Kp * (1 +  Td/T) * error + Kp * (T/Ti - 1 - 2*Td/T) * error_1 + Kp * Td/T * error_2 )
	double constant1;                  // * Kp * (1 + Td/pidSampleTime)
	double constant2;                  // * Kp * (T/Ti - 1 - 2*Td/T)
	double constant3;                  // * Kp * Td/T 

	int controllerDirection;

    double *myPosition;              // * Pointers to the Input, Output, and Setpoint variables
    double *myOutput;             //   This creates a hard link between the variables and the 
    double *mySetpoint;           //   PID, freeing the user from having to constantly tell us
                                  //   what these values are.  with pointers we'll just know.
			  
	unsigned long lastTime;
	double outputSum, error_1, error_2;

	unsigned long SampleTime;
	double outMin, outMax;
	bool inAuto, pOnE;
};

#endif

