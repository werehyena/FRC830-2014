#include "Gamepad.h"
#include "Winch.h"
#include "Arm.h"
#include "Rangefinder.h"
#include "DigitalLED.h"
#include <cmath>

class AerialAssistRobot : public IterativeRobot
{
	//PWM pins
	static const int ROLLER_PWM = 1;
	static const int ARM_LIFT_PWM  = 2;
	static const int LEFT_DRIVE_PWM = 4;
	static const int RIGHT_DRIVE_PWM = 7;
	static const int WINCH_PWM = 9;
	//relay
	static const int COMPRESSOR_RELAY = 1;
	
	//Digital IO pins
	static const int PRESSURE_SWITCH_DIO = 6;
	static const int WINCH_MAX_LIMIT_DIO = 4;
	static const int WINCH_ZERO_POINT_DIO = 8; //not used
	
	static const int ARM_FLOOR_SWITCH_DIO = 5; //not used
	static const int ARM_TOP_SWITCH_DIO = 3;
	static const int ARM_LINE_BREAK_DIO = 9;
 	
	static const int ARM_ENCODER_A_CHANNEL = 1;
	static const int ARM_ENCODER_B_CHANNEL = 2;
	
    static const int RANGE_FINDER_PING_CHANNEL_DIO = 13;
    static const int RANGE_FINDER_ECHO_CHANNEL_DIO = 14;
    
    static const int RED_LED_DIO = 12;
    static const int GREEN_LED_DIO = 10;
    static const int BLUE_LED_DIO = 11;
    
	static const int WINCH_ENCODER_A_CHANNEL = 5; //not used
	static const int WINCH_ENCODER_B_CHANNEL = 8; //not used
	
	//Solenoids
	static const int GEAR_SHIFT_SOL_FORWARD = 8;
	static const int GEAR_SHIFT_SOL_REVERSE = 1;
	static const int CLUTCH_SOL = 2;
	static const DoubleSolenoid::Value LOW_GEAR = DoubleSolenoid::kForward;
	static const DoubleSolenoid::Value HIGH_GEAR = DoubleSolenoid::kReverse;
	static const bool CLUTCH_IN = true;
	static const bool CLUTCH_OUT = false;
	
    static const float MAX_ACCEL_TIME = 0.3f;        //how many seconds we want it to take to get to max speed
    float max_delta_speed;
    //max_delta_speed = 1.0 / (MAX_ACCEL_TIME * GetLoopsPerSec)
    
    static const float FIRING_DISTANCE = 180.0f; //TODO: determine this for real
	
	
    float old_turn, old_forward;
    
    Victor * left_drive;
    Victor * right_drive;
	RobotDrive * drive;
	
	DigitalInput * arm_floor;
	DigitalInput * arm_top;
	DigitalInput * arm_ball;
	Victor * roller;
	Victor * arm_lift;
	Encoder * arm_encoder;
	
	Arm * arm;
	
	Victor * winch_motor;
	Encoder * winch_encoder;
	Winch * winch;
	float winch_rotations;
	bool winch_rot_adjusted;
	DigitalInput * winch_max_switch;
	DigitalInput * winch_zero_switch;
	
	DigitalOutput * led_red_channel;
	DigitalOutput * led_green_channel;
	DigitalOutput * led_blue_channel;
	DigitalLED * led;
	DigitalLED::rgb_color alliance_color;
	
	DoubleSolenoid * gear_shift;
	Solenoid * clutch;
	
	//DigitalInput * pressure_switch;
	Compressor * compressor;
	
	Ultrasonic * ultrasonic;
	Rangefinder * rangefinder;
	
	Timer * timer;
	
	Gamepad * pilot;
	Gamepad * copilot;
	
	//AxisCamera * camera;
	
	DriverStationLCD * lcd;
	DriverStation * ds;

	float clamp(float input, float min){
		if (fabs(input) < fabs(min)){
			return 0.0f;
		} else {
			return input;
		}
	}
	
	
public:
	AerialAssistRobot(void)	{

	}
	
	/********************************** Init Routines *************************************/

	void RobotInit(void) {
		left_drive = new Victor(LEFT_DRIVE_PWM);
		right_drive = new Victor(RIGHT_DRIVE_PWM);
		drive = new RobotDrive(left_drive, right_drive);
		drive->SetInvertedMotor(RobotDrive::kFrontLeftMotor, false);
		drive->SetInvertedMotor(RobotDrive::kRearLeftMotor, false);
		drive->SetInvertedMotor(RobotDrive::kFrontRightMotor, false);
		drive->SetInvertedMotor(RobotDrive::kRearRightMotor, false);
		old_forward = 0.0f;
		old_turn = 0.0f;
		
		roller = new Victor(ROLLER_PWM);
		arm_lift = new Victor(ARM_LIFT_PWM);
		arm_floor = new DigitalInput(ARM_FLOOR_SWITCH_DIO);
		arm_top = new DigitalInput(ARM_TOP_SWITCH_DIO);
		arm_ball = new DigitalInput(ARM_LINE_BREAK_DIO);
		arm_encoder = new Encoder(ARM_ENCODER_A_CHANNEL, ARM_ENCODER_B_CHANNEL);
		
		arm = new Arm(roller, arm_lift, arm_encoder, arm_floor, arm_top, arm_ball);
		
		winch_motor = new Victor(WINCH_PWM);
		winch_encoder = new Encoder(WINCH_ENCODER_A_CHANNEL, WINCH_ENCODER_B_CHANNEL);
		winch_max_switch = new DigitalInput (WINCH_MAX_LIMIT_DIO);
		winch_zero_switch = new DigitalInput (WINCH_ZERO_POINT_DIO);
		
		led_red_channel = new DigitalOutput(RED_LED_DIO);
		led_green_channel = new DigitalOutput(GREEN_LED_DIO);
		led_blue_channel = new DigitalOutput(BLUE_LED_DIO);
		led = new DigitalLED(led_red_channel, led_green_channel, led_blue_channel);
		
		gear_shift = new DoubleSolenoid(GEAR_SHIFT_SOL_FORWARD, GEAR_SHIFT_SOL_REVERSE);
		clutch = new Solenoid(CLUTCH_SOL);
		
		winch = new Winch(winch_motor, clutch, winch_encoder, winch_zero_switch, winch_max_switch);
		
		ultrasonic = new Ultrasonic(RANGE_FINDER_PING_CHANNEL_DIO, RANGE_FINDER_ECHO_CHANNEL_DIO);
		rangefinder = new Rangefinder(ultrasonic);
		
		//pressure_switch = new DigitalInput(PRESSURE_SWITCH_DIO);
		compressor = new Compressor(PRESSURE_SWITCH_DIO, COMPRESSOR_RELAY);
		lcd = DriverStationLCD::GetInstance();
		ds = DriverStation::GetInstance();
		if (ds->GetAlliance() == DriverStation::kBlue){
			alliance_color = DigitalLED::BLUE;
		} else {
			alliance_color = DigitalLED::RED;
		}
		
		//camera = &AxisCamera::GetInstance();
		
		timer = new Timer();
		
		pilot = new Gamepad(1);
		copilot = new Gamepad(2);
		
	}
	
	void DisabledInit(void) {
		lcd->Clear();
	}
	
	void AutonomousInit(void) {
		lcd->Clear();
		AutonomousMainInit();
	}
	
	void AutonomousMainInit(void) {
		clutch->Set(CLUTCH_IN);
		gear_shift->Set(LOW_GEAR);
		winch->wind_back();
		timer->Reset();
		timer->Start();
		compressor->Start(); //required by rules
	}
	
	void AutonomousDriveForwardInit(void) {
		gear_shift->Set(LOW_GEAR);
		compressor->Start();
		timer->Reset();
		timer->Start();
	}

	void TeleopInit(void) {
		compressor->Start(); //required by rules
		lcd->Clear();
		
		/*
		winch_rotations = Winch::STANDARD_ROTATIONS_TARGET;
		winch_rot_adjusted = false;
		winch_encoder->Reset();
		*/
	}

	/********************************** Periodic Routines *************************************/
	
	void DisabledPeriodic(void)  {
		lcd->PrintfLine(DriverStationLCD::kUser_Line1, "disabled");
		led->Set(DigitalLED::OFF);
		lcd->UpdateLCD();
	}

	void AutonomousPeriodic(void) {
		AutonomousMainPeriodic();
	}
	
	void AutonomousMainPeriodic(void) {
		double time_s = timer->Get();
		if (time_s < 5.0){
			arm->drop_ball_in();
		} else if (time_s < 7.0){
			arm->move_down();
		}
		
		if (time_s > 8.5 && time_s < 9.0){
			winch->fire();
			lcd->PrintfLine(DriverStationLCD::kUser_Line4, "firing");
		} else {
			lcd->PrintfLine(DriverStationLCD::kUser_Line4, "");
		}
		if (time_s < 8.0){
			//drive->ArcadeDrive(0.5f, 0.0f);
			drive->ArcadeDrive(0.5f, -0.4f);
			lcd->PrintfLine(DriverStationLCD::kUser_Line5, "left: %f", left_drive->Get());
			lcd->PrintfLine(DriverStationLCD::kUser_Line6, "right: %f", right_drive->Get());
		}
		
		//flash LEDs
		//flash red from (0.0 -> 0.5) and (1.0 -> 1.5), then hold green from 2.0s on
		if(time_s < 0.5 || time_s > 1.0 && time_s < 1.5){
			led->Set(DigitalLED::RED);
		} else if (time_s > 0.5 && time_s < 1.0 || time_s > 1.5 && time_s < 2.0){
			led->Set(DigitalLED::OFF);
		} else {
			led->Set(DigitalLED::GREEN);
		}
		
		arm->update();
		winch->update();
		rangefinder->update();
		lcd->PrintfLine(DriverStationLCD::kUser_Line1, "auton");
		lcd->PrintfLine(DriverStationLCD::kUser_Line2, "time: %f", time_s);
		lcd->PrintfLine(DriverStationLCD::kUser_Line3, "dist: %f", rangefinder->Get());
		lcd->UpdateLCD();
	}

	void AutonomousDriveForwardPeriodic() {
		if (true || timer->Get() < 5.0){
			//drive->ArcadeDrive(0.5f, 0.0f);
			drive->ArcadeDrive(0.5f, -0.4f);
			lcd->PrintfLine(DriverStationLCD::kUser_Line5, "left: %f", left_drive->Get());
			lcd->PrintfLine(DriverStationLCD::kUser_Line6, "right: %f", right_drive->Get());
		}
		rangefinder->update();
		arm->update();
		winch->update();
		lcd->PrintfLine(DriverStationLCD::kUser_Line1, "auton drive");
		lcd->PrintfLine(DriverStationLCD::kUser_Line2, "time: %f", timer->Get());
		lcd->PrintfLine(DriverStationLCD::kUser_Line3, "dist: %f", rangefinder->Get());
		lcd->UpdateLCD();
	}
	
	void TeleopPeriodic(void) {
		//standard arcade drive using left and right sticks
		//clamp the values so small inputs are ignored
		float speed = -pilot->GetLeftY();
		speed = clamp(speed, 0.05f);
		float turn = -pilot->GetRightX();
		turn = clamp(turn, 0.05f);
		
        //lcd->PrintfLine(DriverStationLCD::kUser_Line2, "%f %f", speed, turn);
        
		max_delta_speed = 1.0f / (MAX_ACCEL_TIME * GetLoopsPerSec()); //1.0 represents the maximum victor input
        //float delta_turn = turn - old_turn;
        float delta_speed = speed - old_forward;
        
        if (delta_speed > max_delta_speed){
			speed = old_forward + max_delta_speed;
        } else if (delta_speed < -max_delta_speed){
			speed = old_forward - max_delta_speed;
        }
        //if neither of these is the case, |delta_speed| is less than the max 
        //and we can just use the given value without modification.
         
        lcd->PrintfLine(DriverStationLCD::kUser_Line2, "%f %f", speed, turn);
        //lcd->PrintfLine(DriverStationLCD::kUser_Line3, "%f %f", left_drive->Get(), right_drive->Get());
         
        drive->ArcadeDrive(speed, turn);
        //drive->TankDrive(-pilot->GetLeftY(), pilot->GetRightY());
        old_turn = turn;
        old_forward = speed;
		
 		if (pilot->GetNumberedButton(5) || pilot->GetNumberedButton(6)){
 			gear_shift->Set(HIGH_GEAR);
 		} else if (pilot->GetNumberedButton(7) || pilot->GetNumberedButton(8)){
 			gear_shift->Set(LOW_GEAR);
 		}

		//spin the roller forward or back
		if ((copilot->GetNumberedButton(Gamepad::F310_A))) {
			arm->run_roller_in();
		} else if (copilot->GetNumberedButton(Gamepad::F310_Y)){
			arm->run_roller_out();
		} else if (copilot->GetNumberedButton(Gamepad::F310_X)){
			arm->drop_ball_in();
		}
		
		//this is reversed because katie is weird
		float left_y = clamp(copilot->GetRawAxis(Gamepad::F310_LEFT_Y), 0.05f);
		//lcd->PrintfLine(DriverStationLCD::kUser_Line4, "%arm top: %d", arm_top->Get());
		if (left_y > 0.2f) {
			arm->move_up(left_y);
		} else if (left_y < -0.3f){
			arm->move_down();
		}
		
		lcd->PrintfLine(DriverStationLCD::kUser_Line5, "winch: %d", winch_max_switch->Get());
		if (copilot->GetNumberedButton(Gamepad::F310_B)){
			winch->wind_back();
		}
		
		if (copilot->GetNumberedButton(Gamepad::F310_LB)){
			winch->wind_back();
			arm->load_sequence();
		}
		
		if (copilot->GetNumberedButton(Gamepad::F310_RB) || pilot->GetNumberedButton(2)){
			if (winch_max_switch->Get()){
				lcd->PrintfLine(DriverStationLCD::kUser_Line4, "winch not ready");
			} else {
				lcd->PrintfLine(DriverStationLCD::kUser_Line4, "firing");
			}
			winch->fire();
		} else {
			if (!winch_max_switch->Get()){
				lcd->PrintfLine(DriverStationLCD::kUser_Line4, "winch not ready");
			} else {
				lcd->PrintfLine(DriverStationLCD::kUser_Line4, "ready to fire");
			}
		}
		
		if (fabs(FIRING_DISTANCE - rangefinder->Get()) < 6.0f){
			led->Set(DigitalLED::GREEN);
		} else if (arm->ball_captured()){
			led->Set(DigitalLED::YELLOW); 
		} else {
			led->Set(alliance_color);
		}
		
		//camera->GetImage();
		
		arm->update();
		winch->update();
		rangefinder->update();
		
		lcd->PrintfLine(DriverStationLCD::kUser_Line1, "teleop");
		//lcd->PrintfLine(DriverStationLCD::kUser_Line3, "enc: %d", arm_encoder->Get());
		lcd->PrintfLine(DriverStationLCD::kUser_Line3, "arm: %d", arm_top->Get());
		lcd->PrintfLine(DriverStationLCD::kUser_Line6, "distance: %f", rangefinder->Get());
		lcd->UpdateLCD();
		
	}

	//storing place for stuff not ready to be implemented yet
	void UnusedTeleopPeriodic() {
		//prime shooter to fire
		
		float right_y = -copilot->GetRawAxis(Gamepad::F310_RIGHT_Y);
		if (right_y > 0.3f && !winch_rot_adjusted){
			winch_rotations += Winch::STANDARD_ROTATIONS_INCREMENT;
			winch_rot_adjusted = true;
		} else if (right_y < -0.3f && !winch_rot_adjusted){
			winch_rotations -= Winch::STANDARD_ROTATIONS_INCREMENT;
		} else if (fabs(right_y) <= 0.3 ){
			winch_rot_adjusted = false;
		}
		
		if (copilot->GetNumberedButton(Gamepad::F310_R_STICK)){
			winch_encoder->Start();
			winch->wind_back_rotations(winch_rotations);
		}
		
		float dpad = copilot->GetRawAxis(Gamepad::F310_DPAD_X_AXIS);
		
		if (dpad < -0.5f){
			arm->set_position(Arm::FLOOR_POSITION);
		} else if (dpad > 0.5f){
			arm->set_position(Arm::TOP_POSITION);
		} else if (false) {		//we don't have a control for this right now
			arm->set_position(Arm::LOW_GOAL_POSITION);
		}
		
		//Compressor on button 10
		//flagrantly illegal according to inspector
		/*
		if (pilot->GetNumberedButton(10) && !pressure_switch->Get()){
			compressor->Set(Relay::kOn);
		} else {
			compressor->Set(Relay::kOff);
		}
		*/
	}
	
	bool red;
	bool green;
	bool blue;
	
	void TestInit() {
		lcd->Clear();
		red = false;
		green = false;
		blue = false;
	}
	
	void TestPeriodic() {
		if (copilot->GetNumberedButtonPressed(Gamepad::F310_B)){
			red = !red;
		}
		if (copilot->GetNumberedButtonPressed(Gamepad::F310_A)){
			green = !green;
		}
		if (copilot->GetNumberedButtonPressed(Gamepad::F310_X)){
			blue = !blue;
		}
		led->Set(red, green, blue);
		
		lcd->PrintfLine(DriverStationLCD::kUser_Line1, "test");
		lcd->PrintfLine(DriverStationLCD::kUser_Line2, "r: %d", red);
		lcd->PrintfLine(DriverStationLCD::kUser_Line3, "g: %d", green);
		lcd->PrintfLine(DriverStationLCD::kUser_Line4, "b: %d", blue);
		lcd->UpdateLCD();
		
	}
	
	void TestOneGamepadInit() {
		
	}
	
	void TestOneGamepadPeriodic() {
		
	}
	
	Victor * test_victors[10];
	DigitalOutput * test_dios[14];
	void TestAllInit() {
		for (int i = 0; i < 10; i++){
			test_victors[i] = new Victor(i);
		}
		for (int i = 0; i < 14; i++){
			test_dios[i] = new DigitalOutput(i);
		}
	}
	
	void TestAllPeriodic() {
		for (int i = 0; i < 10; i++){
			test_victors[i]->Set(1.0f);
		}
		for (int i = 0; i < 14; i++){
			test_dios[i]->Set(true);
		}
	}
	
};

START_ROBOT_CLASS(AerialAssistRobot);
