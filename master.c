#include <LiquidCrystal.h> // Includem biblioteca pentru afisajul LCD

// Definim constante pentru calculele umidității
#define H_START -40 // Temperatura de start pentru calculul umidității
#define U_START 68  // Umiditatea de start
#define H_END 60    // Temperatura de sfârșit pentru calculul umidității
#define U_END 100.0 // Umiditatea de sfârșit

// Inițializăm afișajul LCD cu pinii specificați
LiquidCrystal lcd(11, 12, 4, 5, 6, 7);

// Variabile pentru stocarea temperaturii și umidității
int temperatura_citita = 0;  // Temperatura citită de la senzor
int temperatura_maxima = 85; // Temperatura maximă admisă
unsigned int umiditate;

// Funcții și variabile pentru comunicarea I2C

unsigned char Adresa_slave = 0x20, read = 1; // Adresa slave și bitul de read
unsigned char info_primit; // Variabila pentru stocarea informației primite
int info_primit_int = 0; // Variabila pentru stocarea informației primite, convertită în întreg

// Inițializarea modulului I2C ca master
void I2C_Initializare_Master() {
  TWSR = 0x00; // Setăm registrul de stare pe 0 (fără prescalar)
  TWBR = 0x0c; // Setăm SCL la 400 kHz (fără prescalar)
}

// Pornirea comunicației TWI (I2C)
void I2C_Pornire_comunicatie() {
  TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // Setăm controlul comunicației
  while(!(TWCR & (1<<TWINT))); // Așteptăm până când condiția de start a fost transmisă
  while((TWSR & 0xF8) != 0x08); // Verificăm confirmarea
}

void I2C_Citire_Adresa(unsigned char data) {
  TWDR = data; // Setăm adresa și instrucțiunile de citire
  TWCR = (1<<TWINT) | (1<<TWEN); // Activăm TWI
  while (!(TWCR & (1<<TWINT))); // Așteptăm transmiterea datelor
  while((TWSR & 0xF8) != 0x40);  // Verificăm confirmarea
}

// Citire și stocare date primite
void I2C_Citire_Date() {
  TWCR = (1<<TWINT) | (1<<TWEN); // Activăm TWI
  while (!(TWCR & (1<<TWINT))); // Așteptăm transmiterea datelor
  while((TWSR & 0xF8) != 0x58); // Verificăm confirmarea
  info_primit = TWDR; // Stocăm informația primită
}

// Oprire comunicație
void I2C_Oprire_Comunicatie() {
  TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); // Setăm condiția de oprire
  while(!(TWCR & (1<<TWSTO)));  // Așteptăm transmiterea condiției de stop
}


void setup() {
  I2C_Initializare_Master(); // Inițializăm modulul I2C ca master
  lcd.begin(16, 2); // Inițializăm afișajul LCD (16 coloane x 2 rânduri)
  // Configurăm pinii PD2 și PD3 ca intrări și activăm rezistorii de pull-up
  DDRD &= ~((1 << DDD2) | (1 << DDD3)); // Setare PD2 și PD3 ca intrări
  PORTD |= (1 << PORTD2) | (1 << PORTD3); // Activare rezistori de pull-up pentru PD2 și PD3
  // Configurăm și activăm intreruperile externe
  EICRA |= (1 << ISC11) | (1 << ISC10) | (1 << ISC01) | (1 << ISC00);
  EIMSK |= (1 << INT1) | (1 << INT0);
  EIFR |= (0 << INTF1) | (0 << INT0);
  PCICR |= (0 << PCIE2) | (0 << PCIE1) | (0 << PCIE0);
  SREG |= (1 << SREG_I); // Activăm intreruperile globale
}

// Funcțiile de calcul al umidității în funcție de temperatura citită
double calculateHumidityMinus40to0(double temperatura_citita) {
  // Umiditatea crește cu 0.8% pentru fiecare grad Celsius sub 0°C.
  return 0.8 * (temperatura_citita - H_START) + U_START;
}

double calculateHumidity60to85(double temperatura_citita) {
  // Umiditatea scade cu 0.56% pentru fiecare grad Celsius peste 60°C.
  return -0.56 * (temperatura_citita - H_END) + U_END;
}

void loop() {
  // Comunicăm cu dispozitivul slave prin I2C și citim temperatura
  I2C_Pornire_comunicatie(); // Pornim comunicația I2C
  I2C_Citire_Adresa(Adresa_slave + read); // Trimitem adresa slave și cerem citirea
  I2C_Citire_Date(); // Citim datele de la slave
  I2C_Oprire_Comunicatie(); // Oprim comunicația I2C
  
  info_primit_int = (int8_t)info_primit; // Convertim datele primite în întreg
  temperatura_citita = info_primit_int; // Atribuim temperatura citită

  // Limităm temperatura citită la valoarea maximă admisă
  if (temperatura_citita >= temperatura_maxima) {
    temperatura_citita = temperatura_maxima;
  }

  // Calculăm umiditatea în funcție de intervalul de temperatură
  if (temperatura_citita >= H_START && temperatura_citita <= 0) {
    umiditate = calculateHumidityMinus40to0(temperatura_citita);
  } else if (temperatura_citita >= H_END && temperatura_citita <= 85) {
    umiditate = calculateHumidity60to85(temperatura_citita);
  } else if (temperatura_citita > 0 && temperatura_citita < H_END) {
    umiditate = 100; // Umiditatea este maximă între 0°C și 60°C
  }

  lcd.setCursor(4, 0);
  lcd.print("     "); // Curățăm spațiu suficient pentru valoarea temperaturii și simbolul grade
  lcd.setCursor(3, 1);
  lcd.print("     "); // Curățăm spațiu suficient pentru valoarea umidității

  // Presupunem că temperatura_maxima poate fi de cel mult 3 caractere
  lcd.setCursor(10, 1);
  lcd.print("     "); // Curățăm spațiu suficient pentru eticheta "TM" și valoarea temperaturii maxime

  // Afișăm eticheta "TC" și valoarea temperaturii
  lcd.setCursor(0, 0); // Setăm cursorul la începutul primei linii
  lcd.print("TC: ");
  lcd.setCursor(4, 0);
  lcd.print(temperatura_citita);
  lcd.setCursor(7, 0);
  lcd.print(char(176)); // Afișăm simbolul pentru grade
  lcd.setCursor(8, 0);
  lcd.print("C");

  // Afișăm eticheta "U" și valoarea umidității
  lcd.setCursor(0, 1); // Setăm cursorul la începutul celei de-a doua linii
  lcd.print("U: ");
  lcd.setCursor(3, 1);
  lcd.print(umiditate);
  lcd.setCursor(6, 1);
  lcd.print("%");

  // Afișăm eticheta "TM" și valoarea temperaturii maxime
  lcd.setCursor(10, 1); // Setăm cursorul pentru eticheta "TM"
  lcd.print("TM: ");
  lcd.setCursor(13, 1); // Setăm cursorul pentru valoarea temperaturii maxime
  lcd.print(temperatura_maxima);
  delay(500); // O pauză de 500 ms între afișări
}

// Rutine de întrerupere

// Rutina de întrerupere pentru INT0 (pinul 2)
ISR(INT0_vect) {
  SREG &= ~(1 << SREG_I); // Dezactivăm întreruperile globale
  if (temperatura_maxima < 85) {
    temperatura_maxima = temperatura_maxima + 1; // Incrementăm temperatura_maxima
  }
  SREG |= (1 << SREG_I); // Re-activăm întreruperile globale
}

// Rutina de întrerupere pentru INT1 (pinul 3)
ISR(INT1_vect) {
  SREG &= ~(1 << SREG_I); // Dezactivăm întreruperile globale
  if (temperatura_maxima > -40) {
    temperatura_maxima = temperatura_maxima - 1; // Decrementăm temperatura_maxima
  }
  SREG |= (1 << SREG_I); // Re-activăm întreruperile globale
}
