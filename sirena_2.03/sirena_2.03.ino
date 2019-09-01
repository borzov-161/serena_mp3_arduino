
#include <avr/sleep.h>
#include <IRremote.h>
#include <SoftwareSerial.h>//вкл альтернативный вход RX TX//для будущего проекта
SoftwareSerial mySoftwareSerial(7, 6);  // RX, TX
#include <DFRobotDFPlayerMini.h>
DFRobotDFPlayerMini myDFPlayer;

unsigned long  SEREN_time=0;     // таймер работы сирены 
uint8_t  ZonaSeren=1, TrekIR=0, randNumber=0 ; //
 bool Seren_activity=0;      // флаг cирены //
volatile bool FLG2_Sleep=0;  // флаг пульта //
volatile bool FLG_Sleep =1;  // тут "1" для того, чтобы ардуино уснул после стартового трека,а не раньше :)
//------------------------настройки-----------------------------
// раскомментируйте ниже строку, если хотите увидеть коды пульта.
#define DEBUG_TO_COMPORT    // печатаем в порт 

uint8_t Regim=0;             // (0~5) можем задать номер режима -номер папки с треками//"0" это случайный выбор по всем папкам
#define SEREN 2              //№ пина вход сирены
#define RECVR 3              //№ пина вход сирены
#define BUSY  4              //№ пина для контроля плеера
#define Power   8            //№ пина включ унч  и плеер
#define sbros_time 700       //время между писками сирены (время должно быть чуть больше пауз) 
#define max_regim    5       //максимальное количество режимов (количество  папке)
#define volum       30       //громкость
#define ALARM7       7       // это № трекa с мелодиями непрерывного звучания (в каждой папке есть)
#define PapkaPult    8       // это папка с мелодиями от пульта
// тут можно поменять мелодию при первом включении системы//например
#define papkaN      3        // 3-это папка с мелодией старта
#define trek_start  1        // 1-это номер трека  в папке  
//----кнопки пульта считаные заранее (тут свои коды прописываем, в HEX ) ---- //

//-------------------------------------------------------------
IRrecv irrecv(RECVR);           // присваиваем пин приема
decode_results results;         //

void myISR () {FLG_Sleep  = 1;} // не разрешаем снова уснуть пока флаг  =1
void myISR2() {FLG2_Sleep = 1;} // не разрешаем снова уснуть пока флаг2 =1


void setup() {
 #ifdef DEBUG_TO_COMPORT
  Serial.begin(9600);
  Serial.println("start ?");
 // myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
 #endif
 pinMode(BUSY,  INPUT);       //как вход //о состоянии плеера
 pinMode(SEREN, INPUT);       //как вход //внешний сигнал сирены
 pinMode(Power,  OUTPUT);     //как выход, управляем унч и плеером
 digitalWrite(Power, HIGH);   //унч и плеер включ
  irrecv.enableIRIn();         //включаем ифк приемник
  mySoftwareSerial.begin (9600);
  payza(1000); // даем немного времени плееру,загрузиться 
 ADCSRA &= ~(1 << ADEN);      // Отключаем АЦП
  if (!myDFPlayer.begin(mySoftwareSerial)) { // запуск плеера//инициализац//более 2 секунд 
        Serial.println("st ?");
        while(true);
       }
 #ifdef DEBUG_TO_COMPORT
  Serial.println("start OK");
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
 #endif 
 myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
 myDFPlayer.volume(volum);   //громкость (0~30).
 myDFPlayer.EQ(DFPLAYER_EQ_POP);
 myDFPlayer.playFolder(papkaN, trek_start); // играем старт (короткую мелодию из первой папки)
 payza(200);  // даем немного времени,ждем начало проигрывания//уснем после проигрывания мелодии старта
 }

  void loop () {        
if (FLG_Sleep) 
{
 digitalWrite(Power, HIGH);  // включ УНЧ и плеер
 if (!Seren_activity && digitalRead(SEREN))    // после первого писка 
        {ZonaSeren++;                          // считаем количество писков
         Seren_activity=1;                     // есть сигнал! активен вход сирены
         SEREN_time = millis();                // запускаем таймер
        }
   else if (Seren_activity && !digitalRead(SEREN)) Seren_activity=0;//сброс флага//готовися к новому писку сирены   
//-----------------------посчитав колич.коротких сигналов сирены --действуем----------------------------
 if (millis()-SEREN_time > sbros_time && ZonaSeren)  // если более хх мсек нет новых писков
  { 
   if(Regim)  randNumber = Regim;               // если задан режим 
   else randNumber = random(1,max_regim);       // если не задан режим,генерируем случайное число от 1 до 5(max_regim)
        SEREN_time = millis();                  // перезапускаем таймер
   if (!Seren_activity)  myDFPlayer.playFolder(randNumber, ZonaSeren);  // если cирена отпищала -то играем свою мелодию
   else  myDFPlayer.playFolder(randNumber, ALARM7); //если играет непрерывно//играем тревогу //мелодия должна быть длинной 20-30 сек
   // ниже пауза,до старта плеера,но не более "sbros_time", чтобы выйти из цикла,не зависнуть
   while( digitalRead(BUSY)&& millis()-SEREN_time < sbros_time)  {}   //при выходе из цикла,после старта плеера-сброс счетчика сирен
   if  (!digitalRead(BUSY)) ZonaSeren=0;
  }
 else if (digitalRead(BUSY)&& millis()-SEREN_time>sbros_time && !ZonaSeren && !digitalRead(SEREN))  OF_Power ();// если отиграла мелодию 
}
//---------------пульт ------------------------------------------------------------------------
else if (FLG2_Sleep) {
  digitalWrite(Power, HIGH);       // включ УНЧ и плеер
   if (irrecv.decode(&results)) {   // если данные ифк пришли// запоминаем что
       payza(200);   // задержка от двойного срабатывания
      SEREN_time = millis();       // запускаем таймер
   #ifdef DEBUG_TO_COMPORT
    Serial.print("kod= ");Serial.println(results.value, HEX);//
   #endif
 switch ( results.value)  
      { 
   case 0xB2EEDF3D  :TrekIR=1;break; //1// с0 до 9 // 10 мелодий, многовато,но флешка стерпит :)
   case 0xFD00FF  :TrekIR=1;break; //1//
      case 0xC9C3741  :TrekIR=2;break; //2// 
      case 0xFD807F  :TrekIR=2;break; //2// 
   case 0x2C87261  :TrekIR=3;break; //3// 
   case 0xFD40BF  :TrekIR=3;break; //3//  
      case 0x1644C1C1  :TrekIR=4;break; //4//
      case 0xFD20DF  :TrekIR=4;break; //4//
   case 0xA6B913BD  :TrekIR=5;break; //5//
   case 0xFDA05F  :TrekIR=5;break; //5//
      case 0xD0529225  :TrekIR=6;break; //6//
      case 0xFD609F  :TrekIR=6;break; //6//
  case 0x1E90961  :TrekIR=7;break; //7//
  case 0xFD10EF  :TrekIR=7;break; //7//
      case 0x925D5B5D  :TrekIR=8;break; //8//
      case 0xFF38C7  :TrekIR=8;break; //8//
  case 0xCB3D6F7D :TrekIR=9;break; //9//
  case 0xFD50AF  :TrekIR=9;break; //9//
    case 0xFF02FD  :TrekIR=0; myDFPlayer.pause(); break; //ok// стоп
    case 0x25802501  :TrekIR=0; myDFPlayer.pause(); break; //ok// стоп
      case 0x6F5974BD  : // кнопка верх 
               Regim < max_regim ? Regim++ : Regim = 0 ;                     
               TrekIR = Regim+10;  break; 
      case 0x57E346E1  : //кнопка вниз
               Regim > 0 ? Regim-- : Regim = max_regim ;                     
               TrekIR = Regim+10;  break;  
         }
   delay(50);
   irrecv.resume(); //готовимся принять следующую команду        
} 
if(digitalRead(Power))      // если проснулись удачно,обрабатываем код пульта
 {
    if (TrekIR && digitalRead(BUSY) && millis()-SEREN_time > sbros_time) { 
       SEREN_time = millis();                              //запоминаем время первой команды 
       myDFPlayer.playFolder(PapkaPult, TrekIR);           //шлем команды,через промежуток "sbros_time" ,ждем просыпания плеера
       while( digitalRead(BUSY)&& millis()-SEREN_time < sbros_time) {}
       if(!digitalRead(BUSY)) TrekIR=0;                    //сброс номера трека,после старта плеера //*/
      }
    else if (digitalRead(BUSY)&& millis()-SEREN_time > 6*sbros_time) OF_Power (); //если не распознали команду пульта,или пропели,то спать 
 }
 else  digitalWrite(Power, HIGH); // вкл питание если оно не вкл
}
if (!FLG_Sleep && !FLG2_Sleep)
 { //если все условия выше отработали свои задачи (флаги=0)- разрешам спать 
  attachInterrupt(digitalPinToInterrupt(SEREN),myISR, HIGH); // Прерывание по высокому уровню
  attachInterrupt(digitalPinToInterrupt(RECVR),myISR2, LOW);  // Прерывание по низкому  уровню
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);          //Устанавливаем  режим событие но высокому уровню
  sleep_mode();                                 // Переводим МК в спящий режим
  detachInterrupt(digitalPinToInterrupt(SEREN));// Выключаем обработку внешнего прерывания и возвращаемся в то место где уснули
  detachInterrupt(digitalPinToInterrupt(RECVR));// Выключаем обработку внешнего прерывания и возвращаемся в то место где уснули
 }//*/ 
}

void OF_Power (){
  TrekIR=0; ZonaSeren=0;     // cброс счетчиков
  digitalWrite(Power, LOW);  // выкл питание плеера и УНЧ
  delay(50);
  FLG_Sleep = 0; FLG2_Sleep = 0; // спать
 }
void payza(unsigned long _mls){//замена 
    unsigned long _timeout = millis() + _mls; 
    while ( millis() < _timeout)  {};  //пустой цикл . ждем не более time_mls
   }
//КОНЕЦ , А КТО ЧИТАЛ МОЛОДЕЦ  
