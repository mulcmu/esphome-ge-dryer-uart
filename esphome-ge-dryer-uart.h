
//class constructor based off of @mannkind
//ESPHomeRoombaComponent/ESPHomeRoombaComponent.h
//https://github.com/mannkind/ESPHomeRoombaComponent

#define USE_UART_DEBUGGER

#include "esphome.h"
#include <vector>
#include "esphome/components/uart/uart_debugger.h"


static const char *TAG = "component.ge_UART";

class component_geUART : 
        public PollingComponent,
        public UARTDevice,
        public CustomAPIDevice {

  public:
    Sensor *sensor_remainingtime;
    TextSensor *textsensor_dryerState;
    TextSensor *textsensor_dryerSubState;
    TextSensor *textsensor_dryerCycle;
    TextSensor *textsensor_endOfCycle;
    TextSensor *textsensor_DrynessSetting;
    TextSensor *textsensor_HeatSetting;   

    
    static component_geUART* instance(UARTComponent *parent)
    {
        static component_geUART* INSTANCE = new component_geUART(parent);
        return INSTANCE;
    }

    //delay setup() so that dryer boards have booted completely before
    //starting to rx/tx on bus
    float get_setup_priority() const override 
    { 
        return esphome::setup_priority::LATE; 
    }

    void setup() override
    {
        ESP_LOGD(TAG, "setup().");
        
        rx_buf.reserve(128);
        
        textsensor_dryerState->publish_state("Unknown");
        textsensor_dryerCycle->publish_state("Unknown");
        textsensor_dryerSubState->publish_state("Unknown");
        textsensor_endOfCycle->publish_state("Unknown");
        textsensor_DrynessSetting->publish_state("Unknown");
        textsensor_HeatSetting->publish_state("Unknown");   
        
        sensor_remainingtime->publish_state(NAN);
    }

    void update() override
    {

        while ( available() ) {
            read_byte(&b);
           
            if(rx_buf.size() == rx_buf.capacity() )  {
                rx_buf.clear();            
            }
                
        }
        
        write(0xFF);
       

    }

        
  private: 
    uint8_t b=0;
    uint8_t last_b=0;
    unsigned long millisProgress=0;
    uint8_t erd=0;
    
    std::vector<uint8_t> rx_buf; 
    std::vector<uint8_t> tx_buf;
    
    //Hardcoded packets to read these ERDs
    //U+ connect uses 0xBE, dryer sends to 0xBF
    //use 0xBB
    std::vector<uint8_t> erd2000= {0xe2, 0x24, 0xb, 0xbb, 0xf0, 0x1, 0x20, 0x00, 0x47, 0x1b, 0xe3};   //Dryer State
    std::vector<uint8_t> erd2001= {0xe2, 0x24, 0xb, 0xbb, 0xf0, 0x1, 0x20, 0x01, 0x57, 0x3a, 0xe3};   //Sub state
    std::vector<uint8_t> erd2002= {0xe2, 0x24, 0xb, 0xbb, 0xf0, 0x1, 0x20, 0x02, 0x67, 0x59, 0xe3};   //End of cycle
    std::vector<uint8_t> erd2007= {0xe2, 0x24, 0xb, 0xbb, 0xf0, 0x1, 0x20, 0x07, 0x37, 0xfc, 0xe3};   //Cycle time remaining
    std::vector<uint8_t> erd200A= {0xe2, 0x24, 0xb, 0xbb, 0xf0, 0x1, 0x20, 0x0a, 0xe6, 0x51, 0xe3};   //Cycle Setting
    std::vector<uint8_t> erd204D= {0xe2, 0x24, 0xb, 0xbb, 0xf0, 0x1, 0x20, 0x4d, 0xde, 0x72, 0xe3};   //Dryness Setting
    std::vector<uint8_t> erd2050= {0xe2, 0x24, 0xb, 0xbb, 0xf0, 0x1, 0x20, 0x50, 0x1d, 0xee, 0xe3};   //Heat Setting
    
   
    component_geUART(UARTComponent *parent) : PollingComponent(200), UARTDevice(parent) 
    {
        this->sensor_remainingtime = new Sensor();
        this->textsensor_dryerState = new TextSensor();
        this->textsensor_dryerSubState = new TextSensor();
        this->textsensor_dryerCycle = new TextSensor();
        this->textsensor_endOfCycle = new TextSensor();
        this->textsensor_DrynessSetting = new TextSensor();
        this->textsensor_HeatSetting = new TextSensor();        
    }
    
    void process_packet()  {
        if(rx_buf[1]!=0xBB)
            return;
        
        if(rx_buf[4]!=0xF0 || rx_buf[6]!=0x20)
            return;
        
        if(rx_buf.size() > 12)  {
            //0x2000:  E2 BB 0D 24 F0 01 20 00 01 00 E6 88 E3
            if(rx_buf[7]==0x00)  {
                ESP_LOGD(TAG, "erd x2000: %X", rx_buf[9]);
                switch (rx_buf[9])  {
                    case 0x00: //Idle screen off
                    case 0x01: //Standby, display on
                        textsensor_dryerState->publish_state("Off");
                        break;
                    case 0x02: //Run
                        textsensor_dryerState->publish_state("Running");
                        break;                    
                    case 0x03: //Paused
                        textsensor_dryerState->publish_state("Paused");
                        break;                    
                    case 0x04: //EOC<br/>
                        textsensor_dryerState->publish_state("Done");
                        break;                    
                    default:
                        char buf[32];
                        sprintf(buf, "ERD 2000 Unknown %X",rx_buf[9]);
                        textsensor_dryerState->publish_state(buf);
                }
            }
            
            //0x2001:  E2 BB 0D 24 F0 01 20 01 01 00 D1 B8 E3  
            if(rx_buf[7]==0x01)  {
                ESP_LOGD(TAG, "erd x2001: %X", rx_buf[9]);
                switch (rx_buf[9])  {
                    case 0x00: 
                        textsensor_dryerSubState->publish_state("N/A");
                        break;
                    case 0x09:
                        textsensor_dryerSubState->publish_state("Tumble");
                        break;                    
                    case 0x0A: 
                        textsensor_dryerSubState->publish_state("Load Detection");
                        break;                    
                    case 0x80: 
                        textsensor_dryerSubState->publish_state("Drying");
                        break;   
                    case 0x81: 
                        textsensor_dryerSubState->publish_state("Steam");
                        break;   
                    case 0x82: 
                        textsensor_dryerSubState->publish_state("Cool Down");
                        break;   
                    case 0x83: 
                        textsensor_dryerSubState->publish_state("Extended Tumble");
                        break;   
                    case 0x84: 
                        textsensor_dryerSubState->publish_state("Damp");
                        break;  
                    case 0x85: 
                        textsensor_dryerSubState->publish_state("Air Fluff");
                        break;                           
                    default:
                        char buf[32];
                        sprintf(buf, "ERD 2001 Unknown %X",rx_buf[9]);
                        textsensor_dryerSubState->publish_state(buf);
                }
            }
            
            //0x2002:    E2 BB 0D 24 F0 01 20 02 01 00 88 E8 E3
            if(rx_buf[7]==0x02)  {
                ESP_LOGD(TAG, "erd x2002: %X", rx_buf[9]);
                switch (rx_buf[9])  {
                    case 0x00: //Damp
                        textsensor_endOfCycle->publish_state("False");
                        break;   
                    case 0x01: //Damp
                        textsensor_endOfCycle->publish_state("True");
                        break;                           
                    default:
                        char buf[32];
                        sprintf(buf, "ERD 2002 Unknown %X",rx_buf[9]);
                        textsensor_endOfCycle->publish_state(buf);
                }
            }
            
            //DEBUG RX: <E2 BE 0E 24 F0 01 20 07 02 0B EA 72 3F E3 E1>
            //INFO Parsed: <GEAFrame(src=0x24, dst=0xBE, payload=<F0 01 20 07 02 0B EA>, ack=True>
            //INFO Parsed payload: <ERDCommand(command=<ERDCommandID.READ: 0xF0>, erds=[0x2007:<0B EA>])>
            //TODO, this could be an escaped packet for the time which will have erroneous result.            
            if(rx_buf[7]==0x07)  {
                uint16_t seconds = (uint16_t)(rx_buf[9]) << 8 | (uint16_t)rx_buf[10];
                float minutes = seconds / 60.0;
                sensor_remainingtime->publish_state(minutes);
            }
            
            //0x200A:  E2 BB 0D 24 F0 01 20 0A 01 06 41 8F E3
            if(rx_buf[7]==0x0A)  {
                switch (rx_buf[9])  {
                    case 0x89:
                        textsensor_dryerCycle->publish_state("Mixed Load");
                        break;
                    case 0x0D:
                        textsensor_dryerCycle->publish_state("Delicates");
                        break;
                    case 0x80:
                        textsensor_dryerCycle->publish_state("Cottons");
                        break;
                    case 0x0B:
                        textsensor_dryerCycle->publish_state("Jeans");
                        break;
                    case 0x8B:
                        textsensor_dryerCycle->publish_state("Casuals");
                        break;
                    case 0x88:
                        textsensor_dryerCycle->publish_state("Quick Dry");
                        break;
                    case 0x06:
                        textsensor_dryerCycle->publish_state("Towels");
                        break;
                    case 0x04:
                        textsensor_dryerCycle->publish_state("Bulky");
                        break;
                    case 0x05:
                        textsensor_dryerCycle->publish_state("Sanitize");
                        break;
                    case 0x85:
                        textsensor_dryerCycle->publish_state("Air Fluff");
                        break;
                    case 0x8C:
                        textsensor_dryerCycle->publish_state("Warm Up");
                        break;
                    case 0x83:
                        textsensor_dryerCycle->publish_state("Timed Dry");
                        break;
                    default:
                        char buf[32];
                        sprintf(buf, "ERD 200A Unknown %X",rx_buf[9]);
                        textsensor_dryerCycle->publish_state(buf);
                }
                        
            }
        
        
            //0x204D:  E2 BB 0D 24 F0 01 20 4D 01 04 F9 F0 E3  
            if(rx_buf[7]==0x4D)  {
                ESP_LOGD(TAG, "erd x204D: %X", rx_buf[9]);
                switch (rx_buf[9])  {
                    case 0x00: 
                        textsensor_DrynessSetting->publish_state("N/A");
                        break;                       
                    case 0x01: //Damp
                        textsensor_DrynessSetting->publish_state("Damp");
                        break;              
                    case 0x02: //Less Dry
                        textsensor_DrynessSetting->publish_state("Less Dry");
                        break;                    
                    case 0x03: //Dry
                        textsensor_DrynessSetting->publish_state("Dry");
                        break;
                    case 0x04: //More Dry
                        textsensor_DrynessSetting->publish_state("More Dry");
                        break;                    
                    default:
                        char buf[32];
                        sprintf(buf, "ERD 204D Unknown %X",rx_buf[9]);
                        textsensor_DrynessSetting->publish_state(buf);
                }
            }
            
            //0x2050:  E2 BB 0D 24 F0 01 20 50 01 05 E8 E0 E3 E3  
            if(rx_buf[7]==0x50)  {
                ESP_LOGD(TAG, "erd x2050: %X", rx_buf[9]);
                switch (rx_buf[9])  {
                    case 0x01: // Air Fluff
                        textsensor_HeatSetting->publish_state("Air Fluff");
                        break;
                    case 0x03: // Low
                        textsensor_HeatSetting->publish_state("Low");
                        break;
                    case 0x04: // Med 
                        textsensor_HeatSetting->publish_state("Medium");                    
                        break;
                    case 0x05: // High
                        textsensor_HeatSetting->publish_state("High");                    
                        break;
                    default:
                        char buf[32];
                        sprintf(buf, "ERD 2050 Unknown %X",rx_buf[9]);
                        textsensor_HeatSetting->publish_state(buf);
                }                
            }
        
        

        }
        

        
        
    }

};
    
