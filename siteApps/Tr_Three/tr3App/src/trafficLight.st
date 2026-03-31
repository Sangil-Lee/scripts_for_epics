program trafficLight

// PV 연결을 위한 변수 선언
int light_red;
assign light_red to "TrafficLight:Red";

int light_yellow;
assign light_yellow to "TrafficLight:Yellow";

int light_green;
assign light_green to "TrafficLight:Green";

int night;
assign night to "TrafficLight:Night";
monitor night;

float period;
assign period to "TrafficLight:Period";
monitor period;

//evflag	evNight;
//sync	night evNight;


// 점멸 횟수를 카운트하기 위한 변수
int blink_count;

ss traffic_light_sm {

    // 1. 초기화 상태
    state init {
        when () {
            light_red = 0;
            light_yellow = 0;
            light_green = 0;
            pvPut(light_red);
            pvPut(light_yellow);
            pvPut(light_green);
            printf("Traffic Light Sequence Started.\n");
        } state red_state
    }

    // 2. Red 상태 (17초 유지)
    state red_state {
        entry {
            light_red = 1;
            light_yellow = 0;
            light_green = 0;
            pvPut(light_red);
            pvPut(light_yellow);
            pvPut(light_green);
            printf("State: RED (17s)\n");
        }

        when (delay(17.0 * period) && night < 1) {
        } state green_solid

        when (night == 1) {
        } state yellow_blink_on
    }

    // 3. Green 점등 상태 (처음 5초 유지)
    state green_solid {
        entry {
            light_red = 0;
            light_yellow = 0;
            light_green = 1;
            pvPut(light_red);
            pvPut(light_yellow);
            pvPut(light_green);
            printf("State: GREEN Solid (5s)\n");
            
            // 점멸 카운트 초기화
            blink_count = 0; 
        }
        when (delay(5.0 * period) && night < 1) {
        } state green_blink_off

        when (night == 1) {
        } state yellow_blink_on
    }

    // 4. Green 점멸 - 꺼짐 상태 (0.5초)
    state green_blink_off {
        entry {
            light_green = 0;
            pvPut(light_green);
            printf("State: GREEN Blink Off (.5s)\n");
        }

        when (delay(0.5) && night < 1) {
        } state green_blink_on

        when (night == 1) {
        } state yellow_blink_on
    }

    // 5. Green 점멸 - 켜짐 상태 (0.5초)
    state green_blink_on {
        entry {
            light_green = 1;
            pvPut(light_green);
            printf("State: GREEN Blink On (.5s)\n");
            blink_count++; // 켜질 때마다 카운트 1 증가
        }
        // 총 5회 점멸 (0.5초 off + 0.5초 on = 1초) * 5 = 5초 달성 시 Yellow로 이동
        when (delay(0.5) && blink_count >= 5 && night < 1) {
        } state yellow_state
        
        // 아직 5회가 안 되었다면 다시 꺼짐 상태로 이동
        when (delay(0.5) && blink_count < 5 && night < 1) {
        } state green_blink_off

        when (night == 1) {
        } state yellow_blink_on
    }

    // 6. Yellow 상태 (3초 유지)
    state yellow_state {
        entry {
            light_red = 0;
            light_yellow = 1;
            light_green = 0;
            pvPut(light_red);
            pvPut(light_yellow);
            pvPut(light_green);
            printf("State: YELLOW (3s)\n");
        }
        when (delay(3.0 * period) && night < 1) {
        } state red_state

        when (night == 1) {
        } state yellow_blink_on
    }

    state yellow_blink_on {
        entry {
            light_yellow = 1;
            pvPut(light_yellow);
            light_red = 0;
            light_yellow = 0;
            light_green = 0;
            pvPut(light_red);
            pvPut(light_yellow);
            pvPut(light_green);
            printf("State: Yello Blink On - Night (.5s)\n");
        }
        when (delay(0.5) && night == 1) {
        } state yellow_blink_off

        when (night < 1 ) {
        } state red_state
    }

    state yellow_blink_off {
        entry {
            light_yellow = 0;
            pvPut(light_yellow);
            printf("State: Yello Blink Off - Night (.5s)\n");
        }
        when (delay(0.5) && night == 1 ) {
        } state yellow_blink_on

        when (night < 1 ) {
        } state red_state
    }
}
