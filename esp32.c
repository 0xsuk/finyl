const int pots_size = 4;
int pots[pots_size] = {32, 35, 33, 34};
int pot_threshold = 100;
int pots_sprev[pots_size] = {0};

const int buttons_size = 11;
byte buttons[buttons_size] = {2, 5, 12, 13, 14, 15, 25, 26, 0, 4, 27};
const int DEBOUNCE_DELAY = 50;
bool lastSteadyStates[buttons_size] = {false};
bool lastFlickerableStates[buttons_size] = {false};
long lastDebouceTimes[buttons_size] = {0};

template <typename T>
void print(T v) {
  Serial.print(v);
}

template <typename T>
void println(T v) {
  Serial.print(v);
  Serial.print("\n");
}

void read_buttons() {
  long now = millis();
  for (int i = 0; i<buttons_size; i++) {
    int currentState = digitalRead(buttons[i]);
    if (currentState != lastFlickerableStates[i]) {
      lastDebouceTimes[i] = now;
      lastFlickerableStates[i] = currentState;
    }
    
    if ((now - lastDebouceTimes[i]) > DEBOUNCE_DELAY) {
      if (lastSteadyStates[i] == HIGH && currentState == LOW) {
        print("button");
        print(i);
        println(":on");
      } else if (lastSteadyStates[i] == LOW && currentState == HIGH) {
        print("button");
        print(i);
        println(":off");
      }

      lastSteadyStates[i] = currentState;
    }
  }
}

void init_potmeters() {
  for (int i = 0; i<pots_size; i++) {
    int v = analogRead(pots[i]);
    print("pot");
    print(i);
    print(":");
    println(v);
    pots_sprev[i] = v;
  }
}

void read_potmeters() {
  for (int i = 0; i<pots_size; i++) {
    int v = analogRead(pots[i]);
    if (abs(v-pots_sprev[i]) > pot_threshold) {
      print("pot");
      print(i);
      print(":");
      println(v);

      pots_sprev[i] = v;
    }
  }
}

void setup() {
  Serial.begin(9600);
  for (int i = 0; i<buttons_size; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }
  init_potmeters();
}

void loop() {
  if (Serial.available() > 0) {
    String s = Serial.readString();
    s.trim();
    print("received:");
    println(s);
    if (s == "init") {
      init_potmeters();
      println("initialized\n");
    }
  }
  
  read_buttons();
  read_potmeters();
}
