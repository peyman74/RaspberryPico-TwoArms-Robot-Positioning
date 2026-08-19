#include "PID_vel.h"

//Constructor *********************************************************
PID_vel::PID_vel(double* Feedback, double* Output, double* Setpoint,
        double Kp, double Ti, double Td, int POn, int ControllerDirection)
{
    myOutput = Output;
    myPosition = Feedback;
    mySetpoint = Setpoint;
    inAuto = false;
    controllerDirection = ControllerDirection;  // Store but not used in velocity form
    error_1 = 0;
    error_2 = 0;
    outputSum = 0;

    PID_vel::SetOutputLimits(-255, 255);				//default output limit corresponds to
												//the pico pwm limits

    SampleTime = 100;							//default Controller Sample Time is 0.1 seconds
    PID_vel::SetTunings(Kp, Ti, Td);

    lastTime = millis()-SampleTime;
}
PID_vel::PID_vel(double* Feedback, double* Output, double* Setpoint,
        double Kp, double Ti, double Td)
    :PID_vel::PID_vel(Feedback, Output, Setpoint, Kp, Ti, Td, 0, DIRECT)  // Default values for unused params
{

}
void PID_vel::SetTunings(double Kp, double Ti, double Td)
{
   if (Kp<0 || Ti<0 || Td<0) return;

   dispKp = Kp, dispKi = Kp/Ti, dispKd = Kp*Td;
   //Calculate constances in order not to do mathematics in each Compute call
   // output += Kp * (1 +  Td/T) * error + Kp * (T/Ti - 1 - 2*Td/T) * error_1 + Kp * Td/T * error_2 ); or
   // output += constant1 * error + constant2 * error_1 + constant3 * error_2 ; 
   constant1 = Kp * (1 + Td/SampleTime);
   if (Ti != 0) constant2 =  Kp * (SampleTime/Ti - 1 - 2*Td/SampleTime);
   else constant2 = 0;
   constant3 = Kp * Td/SampleTime;
}



/* SetSampleTime(...) *********************************************************
 * sets the period, in Milliseconds, at which the calculation is performed
 ******************************************************************************/
void PID_vel::SetSampleTime(int NewSampleTime)
{
   if (NewSampleTime > 0)
   {
      SampleTime = (unsigned long)NewSampleTime;
   }
}
void PID_vel::SetOutputLimits(double Min, double Max)
{
   if(Min >= Max) return;
   outMin = Min;
   outMax = Max;

   if(inAuto)
   {
	   if(*myOutput > outMax) *myOutput = outMax;
	   else if(*myOutput < outMin) *myOutput = outMin;

	   if(outputSum > outMax) outputSum= outMax;
	   else if(outputSum < outMin) outputSum= outMin;
   }
}
void PID_vel::SetMode(int Mode)   
{
    bool newAuto = (Mode == AUTOMATIC);
    if(newAuto && !inAuto)
    {  /*we just went from manual to auto*/
        PID_vel::Initialize();  // if the mode is changed from Manual to Automatic, initialize the controller
    }
    inAuto = newAuto;
}
/* Initialize()****************************************************************
 *	does all the things that need to happen to ensure a bumpless transfer
 *  from manual to automatic mode.
 ******************************************************************************/
void PID_vel::Initialize()
{
   outputSum = 0;
   error_1 = 0;
   error_2 = 0;
}
bool PID_vel::Compute()
{
  if(!inAuto) return false;
  unsigned long now = millis();
  unsigned long timeChange = (now - lastTime);
  if(timeChange>=SampleTime)
  { 
    double feedback = *myPosition;
    double error = *mySetpoint - feedback;
    double  output = 0;
    // PID calculations diffrent method 
    //outputSum +=   Kp * ((1 + Ki + Kd) * error - (1 + 2 * Kd) * error_1 + Kd * error_2 );  // PID formula right point integration
    //outputSum += Kp * ( (1 +  Kd) * error + (Ki - 1 - 2*Kd) * error_1 + Kd * error_2 );  // PID formula left point integration
     outputSum += constant1 * error + constant2 * error_1 + constant3 * error_2 ;

    //if(outputSum > outMax) outputSum= outMax;
    //else if(outputSum < outMin) outputSum= outMin;  
    output = constrain(outputSum, outMin, outMax);   
    *myOutput = output;    
     error_2 = error_1;
     error_1 = error;
    /*if (output = 0) {
      outputSum = 0;
      error_1 = 0;
      error_1 = 0;
    } */

    lastTime = now;
    return true;
  }
  else return false;    
}

double PID_vel::GetKp(){ return  constant1; }
double PID_vel::GetKi(){ return  constant2;}
double PID_vel::GetKd(){ return  constant3;}
int PID_vel::GetMode(){ return  inAuto ? AUTOMATIC : MANUAL;}