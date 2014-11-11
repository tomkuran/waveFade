/*

 */
#define TRIGER_ON 1
#define TRIGER_OFF 0
#define PIR_UP 2
#define PIR_DN 1
#define NO_PIR 0



byte timer = 0;           // The higher the number, the slower the timing.
byte onTime = 20;         //

byte stepDelay = 9;

byte stairsPins[] = { 
  44,2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}; // an array of  PWM pin numbers to which LEDs are attached
byte stairsBrightness[] = { 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
byte stairsCount = 13;       // the number of pins (i.e. the length of the array)

byte railPin = 45;
byte dimmRail = 0;
byte railBrightness = 0;
boolean dimFlg = false;

byte photoPin = A15;


short trigerOnUp = 0;
short trigerOffUp = 0;
short trigerOnDown = 0;
short trigerOffDown = 0; 
short frameOnUp = 0;
short frameOnDown = 0;
short frameOffUp = 0;
short frameOffDown = 0;



byte waveFormT[13][196];
byte offset[196];

//pin for downStars PIR with interrupt
byte downstairsPIR = 18;

//pin for upStars PIR with interrupt
byte upstairsPIR = 19;

byte lastSeenPir = NO_PIR;

void setup() {

  Serial.begin(9600);

  // the array elements are numbered from 0 to (stairsCount - 1).
  // use a for loop to initialize each pin as an output:
  for (int thisPin = 0; thisPin < stairsCount; thisPin++) {
    pinMode(stairsPins[thisPin], OUTPUT);
  }

  pinMode(downstairsPIR, INPUT);
  pinMode(upstairsPIR, INPUT);

  pinMode(photoPin, INPUT);

  //initialize railPin as output
  pinMode(railPin, OUTPUT);

  //wywolanie funkcji obliczajacej tablice animacji
  setupWaveFormT();

  //attaching interrupts
  attachInterrupt(5, downstairsPirIsr, RISING); 
  attachInterrupt(4, upstairsPirIsr, RISING);   
}

//obsługa przerwania czujki na dole schodów
void downstairsPirIsr() {
  noInterrupts();
  {
    //triger wlacz w gore
    trigerOnUp = 1;

    //reset timera
    setTimer();

    //ostatnio widziana czujka
    lastSeenPir = PIR_DN;

    //reset wygaszania
    resetFramesCount();
  }
  interrupts();
}


//analogiczna obsługa czujki na górze schodów
void upstairsPirIsr() {
  noInterrupts();
  {
    //triger wlacz w dol
    trigerOnDown = 1;

    //reset timera
    setTimer();

    //ostatnio widziana czujka
    lastSeenPir = PIR_UP;

    //reset wygaszania
    resetFramesCount();
  }
  interrupts();
}

void resetFramesCount(){
  frameOffUp = 0;
  frameOffDown = 0;
  trigerOffUp = TRIGER_OFF;
  trigerOffDown = TRIGER_OFF;
}

void setTimer(){
  if(lastSeenPir == NO_PIR){
    timer = 5 * onTime;  
  }
  else{
    timer = onTime;  
  }
  //  Serial.println(timer);
}

//SPRAWDZONA
//FUNKCJA WŁĄCZA FALĘ ŚWIETLNĄ
void waveLightOn() {

  //sprawdzamy czy to nie koniec animacji, jezeli tak to zerujemy liczniki klatek i flagę pirEvent
  if(frameOnDown + frameOnUp >= 246 || frameOnDown >= 195
    || frameOnUp >= 195){
    trigerOnUp = TRIGER_OFF;
    trigerOnDown = TRIGER_OFF;
    frameOnDown = 0;
    frameOnUp = 0;
    return;
  }

  //Mamy tablicę z zapisanymi klatkami animacji (jednowymiarowe)
  //w kolejnych przebiegach wyświetlamy kolejne klatki, ale klatki różnią sie na max 5 polach
  //Tablica offset zawiera informacje o tym które pola zmieniają się w danej klatce obrazu
  //offset[frameOnXXX] jest tym polem od którego zaczynamy zmiany
  if(trigerOnUp==TRIGER_ON){
    for (byte x = offset[frameOnUp]; (x < offset[frameOnUp]+5) && (x<13); x++) {
      if(stairsBrightness[x] < waveFormT[x][frameOnUp]){
        stairsBrightness[x] = waveFormT[x][frameOnUp];
        analogWrite(stairsPins[x], waveFormT[x][frameOnUp]);
      }
    }
    frameOnUp++;
    railOn();
  }

  //zapalanie w dół (tylko 5 w przebiegu petli)
  if(trigerOnDown==TRIGER_ON){
    for (byte x = offset[frameOnDown]; (x < offset[frameOnDown]+5) && (x<13); x++) {
      if(stairsBrightness[12-x] < waveFormT[x][frameOnDown]){
        stairsBrightness[12-x] = waveFormT[x][frameOnDown];
        analogWrite(stairsPins[12-x], stairsBrightness[12-x]);
      }
    }
    frameOnDown++;
    railOn();
  }
  delay(stepDelay);
}



//Funkcja wygaszająca
//sprawdzamy czy mozna rozpocząć wyłączanie stopni
//jezeli tak to kopiujemy lastSeenPir do offDirection
//offDirection !=0 oznacza że należy rozpocząć wyłączanie
//offDirection jest potrzebne bo w czasie wygaszania moze nastąpić nowe przerwanie z czujki

void waveLightOff() {

  //timer = 1 oznacza ze czas przygotowac sie do wyłączenia.
  if(timer == 1 && (trigerOffUp + trigerOffDown)==TRIGER_OFF){
    if(lastSeenPir == PIR_UP){
      trigerOffUp = 1;
    } 
    else {
      trigerOffDown = 1;
    }
    lastSeenPir = NO_PIR;
  }

  //JEŻELI TIMER >1 to nie mamy tu nic do roboty

  if(timer >1 || (trigerOffUp + trigerOffDown)==0){
    return;
  }

  //ZEROWANIE TIMERA PO PEŁNYM WYGASZENIU
  if(frameOffUp >= 196 || frameOffDown >= 196){
    frameOffUp = 0;
    frameOffDown = 0;
    trigerOffUp = TRIGER_OFF;
    trigerOffDown = TRIGER_OFF;
    timer=0;
    return;
  }

  //WŁAŚCIWE WYGASZANIE z GORY na DOL
  if(trigerOffUp == TRIGER_ON){
    for (byte x = offset[frameOffUp]; (x < offset[frameOffUp]+5) && (x<13); x++) {
      if(stairsBrightness[x] > 255 - waveFormT[x][frameOffUp]){
        stairsBrightness[x] = 255 -  waveFormT[x][frameOffUp];
        analogWrite(stairsPins[x], stairsBrightness[x]);
      }
    }
    frameOffUp++;
    railOff();
  }

  //WŁAŚCIWE WYGASZANIE Z DOLU DO GORY
  if(trigerOffDown == TRIGER_ON){
    for (byte x = offset[frameOffDown]; (x < offset[frameOffDown]+5) && (x<13); x++) {
      if(stairsBrightness[12-x] > 255 - waveFormT[x][frameOffDown]){
        stairsBrightness[12-x] = 255 - waveFormT[x][frameOffDown];
        analogWrite(stairsPins[12-x], stairsBrightness[12-x]);
      }
    }
    frameOffDown++;
    railOff();
  }

  //OPOZNIENIE
  delay(stepDelay);


}

// FUNKCJA PRZYGOTOWUJĄCA KLATKI DO ANIMACJI
void setupWaveFormT() {
  int y=0;
  float z = 0;
  for (int time = 0; time < 196; time++) {
    offset[time]=0;
    for (byte x = 0; (x < 13); x++) {
      //y = -5 *  (12*x - time);
      z = x - (0.1*time);
      y = -2 * (z*z*z);
      if(y<0)
        y=0;
      if(y>=255){
        y=255;
        offset[time]=x;
      }
      waveFormT[x][time] =  y;
    }
  }

}

void railOnDark(){

  if(analogRead(photoPin)<600){
    dimmRail = 25;
    if(!dimFlg){
      analogWrite(railPin,dimmRail);
      dimFlg = !dimFlg;
    }
  }
  if(analogRead(photoPin)>700){
    dimmRail = 0;
    if(dimFlg){
      analogWrite(railPin,dimmRail);
      dimFlg = !dimFlg;
    }
  }

}

void railOn(){
  if(railBrightness < 250)
    railBrightness+=25;
}

void railOff(){
  if(railBrightness > dimmRail)
    railBrightness-=25;
}

void loop() {

  //proba wlaczenia swiatel
  waveLightOn();

  //proba wylaczenia swiatel
  waveLightOff();

  railOnDark();

  //ODLICZANIE ROZPOCZYNAMY PO USTANIU SYGNAŁÓW Z CZUJEK I PO PEŁNYM ZAPALENIU OSWIETLENIA
  if (timer > 1  && digitalRead(upstairsPIR) == LOW && digitalRead(downstairsPIR) == LOW && ((trigerOnUp + trigerOnDown) == TRIGER_OFF) ){
    timer--;
    delay(100);
    //Serial.println(analogRead(photoPin));
  }
}














