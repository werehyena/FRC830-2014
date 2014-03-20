#ifndef ARM_H_
#define ARM_H_

#include "WPILib.h"

class Arm {
private:
	static const float MOVEMENT_RATE = (90.0f / 4.0f); //going down
	Victor * roller;
	Victor * pivot;
	Encoder * encoder;
	DigitalInput * floor_switch;
	DigitalInput * top_switch;
	DigitalInput * ball_switch;
	PIDController * pid;
	bool roller_set;
	bool pivot_set;
	
public:
	static const int TOP_POSITION = 0;
	static const int FLOOR_POSITION = 67;
	static const int LOW_GOAL_POSITION = 20; //TODO: determine this
	Arm(Victor * roller_motor, Victor * pivot_motor, Encoder * enc, DigitalInput * floor, DigitalInput * top, DigitalInput * ball);
	void set_position(int pos); //position from 0 (floor) to 100 (maximum)
	void run_roller_in();
	void run_roller_out();
	void drop_ball_in();
	void load_sequence();
	void move_up(float speed);
	void move_down();
	void move_up_pid();
	void move_down_pid();
	void hold_position_pid();
	bool ball_captured();
	void update();
};


#endif
