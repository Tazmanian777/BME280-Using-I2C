unsigned int Valoare_potentiometru = 0;  // valoarea citită din ADC
unsigned char temperatura;

// Funcții pentru I2C

// Inițializare slave
void Initializare_Slave() {  // Funcție de inițializare a slave-ului
    TWAR = 0x20;  // Adresa slave-ului
}

// Transmitere de la slave la master
void Scrie_catre_master() {
    // Transmite temperatura
    TWDR = temperatura;
    TWCR = (1 << TWEN) | (1 << TWINT);  // Pornire transmisie
    while ((TWSR & 0xF8) != 0xC0);  // Așteptare ACK pentru temperatura
}

// Așteptare cerere de transmitere de la master
void Asteapta_comanda_master() {  // Funcție care așteaptă o comandă de transmitere date
    while ((TWSR & 0xF8) != 0xA8) {  // Se repetă până când informația corectă a fost primită
        // Primește informația și se dezactivează întreruperea
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);
        while (!(TWCR & (1 << TWINT)));  // Așteaptă până când flag-ul este setat
    }
}

void setup() {
    // Inițializare ADC pentru citire potențiometru
    initializareADC();
    // Inițializare în modul slave
    Initializare_Slave();
}

// Funcție citire pin A0 potențiometru
unsigned int citesteADC() {
    ADMUX = 0x00000000;

    ADCSRA |= (1 << ADSC); //incepem conversia pentru ADC
    // Așteptare conversie
    while ((ADCSRA & (1 << ADIF)) == 0) {}
    ADCSRA |= (1 << ADIF);

    return ADCW;
}

// Funcție inițializare convertor analogic-digital
void initializareADC() {
    // Dezactivare buffer
    DIDR0 = (0 << ADC5D) | (0 << ADC4D) | (0 << ADC3D) | (0 << ADC2D) | (0 << ADC1D) | (0 << ADC0D);
    ADMUX = 0x00000000;
    // Configurare registru de control
    ADCSRA = (1 << ADEN) | (0 << ADSC) | (1 << ADATE) | (0 << ADIF) | (0 << ADIE) | (1 << ADPS2) | (0 << ADPS1) | (0 << ADPS0);
}

// Funcție de mapare a unei valori pe alta
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void loop() {
    Valoare_potentiometru = citesteADC();  // Citim valoarea de la ADC
    temperatura = map(Valoare_potentiometru, 0, 1023, -40, 85);// mapam valoarea de la adc pe una de la -40 la 85 si 

    Asteapta_comanda_master();  // Funcție pentru așteptarea comenzii de la master
    Scrie_catre_master();       // Funcție pentru transmiterea datelor la master
}
