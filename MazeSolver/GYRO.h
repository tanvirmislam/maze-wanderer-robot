
//Gyro - Arduino UNO R3
//VCC  -  5V
//GND  -  GND
//SDA  -  A4
//SCL  -  A5
//INT - port-2

#include <Wire.h>
//Declaring some global variables
int     gyro_x, gyro_y, gyro_z;
long    gyro_x_cal, gyro_y_cal, gyro_z_cal;
boolean set_gyro_angles;

long    acc_x, acc_y, acc_z, acc_total_vector;
float   angle_roll_acc, angle_pitch_acc;

float   angle_pitch, angle_roll;
int     angle_pitch_buffer, angle_roll_buffer;
float   angle_pitch_output, angle_roll_output;

long loop_timer;
int temp;

void setup_mpu_6050_registers(){
    //Activate the MPU-6050
    Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
    Wire.write(0x6B);                                                    //Send the requested starting register
    Wire.write(0x00);                                                    //Set the requested starting register
    Wire.endTransmission();                                             
    
    //Configure the accelerometer (+/-8g)
    Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
    Wire.write(0x1C);                                                    //Send the requested starting register
    Wire.write(0x10);                                                    //Set the requested starting register
    Wire.endTransmission();                                             
    
    //Configure the gyro (500dps full scale)
    Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
    Wire.write(0x1B);                                                    //Send the requested starting register
    Wire.write(0x08);                                                    //Set the requested starting register
    Wire.endTransmission();                                             
}


void read_mpu_6050_data(){                                             //Subroutine for reading the raw gyro and accelerometer data
    int  gyro_x_temp[5];
    int  gyro_y_temp[5];
    int  gyro_z_temp[5];
    
    long acc_x_temp[5];
    long acc_y_temp[5];
    long acc_z_temp[5];
    
    int  gyro_x_sum = 0;
    int  gyro_y_sum = 0;
    int  gyro_z_sum = 0;
    
    long acc_x_sum = 0;
    long acc_y_sum = 0;
    long acc_z_sum = 0;
    
    for (int i = 0; i < 5; i++) {
        Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
        Wire.write(0x3B);                                                    //Send the requested starting register
        Wire.endTransmission();                                              //End the transmission
        Wire.requestFrom(0x68,14);                                           //Request 14 bytes from the MPU-6050
        
        while(Wire.available() < 14);                                        //Wait until all the bytes are received
        
        acc_x_temp[i] = Wire.read()<<8|Wire.read();                                  
        acc_y_temp[i] = Wire.read()<<8|Wire.read();                                  
        acc_z_temp[i] = Wire.read()<<8|Wire.read();                                  
        
        temp = Wire.read()<<8|Wire.read();                                   
        
        gyro_x_temp[i] = Wire.read()<<8|Wire.read();                                 
        gyro_y_temp[i] = Wire.read()<<8|Wire.read();                                 
        gyro_z_temp[i] = Wire.read()<<8|Wire.read();     
    }
    
    for (int i = 0; i < 5; i++) {
        gyro_x_sum += gyro_x_temp[i];
        gyro_y_sum += gyro_y_temp[i];
        gyro_z_sum += gyro_z_temp[i];
        
        acc_x_sum += acc_x_temp[i];
        acc_y_sum += acc_y_temp[i];
        acc_z_sum += acc_z_temp[i];    
    }
    
    gyro_x = gyro_x_sum / 5;
    gyro_y = gyro_y_sum / 5;
    gyro_z = gyro_z_sum / 5;
    
    acc_x = acc_x_sum / 5;
    acc_y = acc_y_sum / 5;
    acc_z = acc_z_sum / 5;
}



void initialize_gyro() {
    Wire.begin();    
    setup_mpu_6050_registers();

    for (int cal_int = 0; cal_int < 1000 ; cal_int ++) {
        read_mpu_6050_data();
        gyro_x_cal += gyro_x;
        gyro_y_cal += gyro_y;
        gyro_z_cal += gyro_z;
        delay(3);
    }

    gyro_x_cal /= 1000;
    gyro_y_cal /= 1000;
    gyro_z_cal /= 1000;
}


float getAngle() {
    read_mpu_6050_data();
    gyro_x -= gyro_x_cal;
    gyro_y -= gyro_y_cal;
    gyro_z -= gyro_z_cal;

    //Gyro angle calculations . Note 0.0000611 = 1 / (250Hz x 65.5)
    angle_pitch += gyro_x * 0.0000611;                                   //Calculate the traveled pitch angle and add this to the angle_pitch variable
    angle_roll  += gyro_y * 0.0000611;                                    //Calculate the traveled roll angle and add this to the angle_roll variable
    
    //0.000001066 = 0.0000611 * (3.142(PI) / 180degr) The Arduino sin function is in radians
    
    angle_pitch += angle_roll * sin(gyro_z * 0.000001066);               //If the IMU has yawed transfer the roll angle to the pitch angel
    angle_roll  -= angle_pitch * sin(gyro_z * 0.000001066);               //If the IMU has yawed transfer the pitch angle to the roll angel
    
    //Accelerometer angle calculations
    acc_total_vector = sqrt((acc_x*acc_x)+(acc_y*acc_y)+(acc_z*acc_z));  //Calculate the total accelerometer vector
    //57.296 = 1 / (3.142 / 180) The Arduino asin function is in radians
    
    angle_pitch_acc = asin((float)acc_y/acc_total_vector)* 57.296;       //Calculate the pitch angle
    angle_roll_acc  = asin((float)acc_x/acc_total_vector)* -57.296;       //Calculate the roll angle
    
    angle_pitch_acc -= 0.0;                                              //Accelerometer calibration value for pitch
    angle_roll_acc -= 0.0;                                               //Accelerometer calibration value for roll
    
    if(set_gyro_angles){                                                 //If the IMU is already started
        angle_pitch = angle_pitch * 0.9996 + angle_pitch_acc * 0.0004;     //Correct the drift of the gyro pitch angle with the accelerometer pitch angle
        angle_roll = angle_roll * 0.9996 + angle_roll_acc * 0.0004;        //Correct the drift of the gyro roll angle with the accelerometer roll angle
    }
    else{                                                                //At first start
        angle_pitch = angle_pitch_acc;                                     //Set the gyro pitch angle equal to the accelerometer pitch angle 
        angle_roll = angle_roll_acc;                                       //Set the gyro roll angle equal to the accelerometer roll angle 
        set_gyro_angles = true;                                            //Set the IMU started flag
    }
    
    //To dampen the pitch and roll angles a complementary filter is used
    angle_pitch_output = angle_pitch_output * 0.9 + angle_pitch * 0.1;   //Take 90% of the output pitch value and add 10% of the raw pitch value
    angle_roll_output  = angle_roll_output * 0.9 + angle_roll * 0.1;      //Take 90% of the output roll value and add 10% of the raw roll value
    

    return (angle_pitch_output);
}
