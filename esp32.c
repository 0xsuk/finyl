const int button1 = 12;
const int pot1 = 32;

bool button1_pressed = false;
bool prev_button1_pressed = false;

template <typename T>
void print(T v) {
  Serial.print(v);
}

template <typename T>
void println(T v) {
  Serial.print(v);
  Serial.print("\n");
}

void read_button() {
  if (digitalRead(button1) == HIGH) {
    button1_pressed = false;
  } else {
    button1_pressed = true;
  }
}

void read_potmeter() {
  int v = analogRead(pot1);
  print("analog:");
  println(v);
}

void send() {
  if (prev_button1_pressed != button1_pressed) {
    if (button1_pressed) {
      println("yes");
      /* Serial.write(0b10000000); */
      prev_button1_pressed = true;
    } else {
      println("no");
      /* Serial.write(0b00000000); */
      prev_button1_pressed = false;
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(button1, INPUT_PULLUP);
}

void loop() {
  read_button();
  read_potmeter();
  send();
}
