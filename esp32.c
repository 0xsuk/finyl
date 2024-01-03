const int pot1 = 32;

const int pots_size = 2;
int pots[pots_size] = {32, 33};
int pot_threshold = 20;
int pots_sprev[pots_size] = {0};

const int buttons_size = 5;
int buttons[buttons_size] = {0, 13, 23, 4, 14};
bool buttons_sprev[buttons_size] = {false};
bool buttons_s[buttons_size] = {false};

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
  for (int i = 0; i<buttons_size; i++) {
    if (digitalRead(buttons[i]) == HIGH) {
      buttons_s[i] = false;
    } else {
      buttons_s[i] = true;
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

void send() {
  bool some = false;
  for (int i = 0; i<buttons_size; i++) {
    if (buttons_sprev[i] != buttons_s[i]) {
      if (buttons_s[i]) {
        print("button");
        print(i);
        println(":y");
        buttons_sprev[i] = true;
        some = true;
      } else {
        print("button");
        print(i);
        println(":n");
        buttons_sprev[i] = false;
      }
    }
  }

  if (some) {
    delay(50);
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
  send();
}
