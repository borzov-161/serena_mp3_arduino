/*
https://github.com/z3t0/Arduino-IRremote
https://github.com/DFRobot/DFRobotDFPlayerMini
https://github.com/vancegroup-mirrors/avr-libc
https://wiki.iarduino.ru/page/ik-priemnik/
тут будет ссыка на YouTube
https://www.drive2.ru/r/lada/456924323106523636/
https://www.drive2.ru/users/borzov161/#blog
*/
/*
 в корне флешки создаем папки "01" "02" "03" "04" "05" "08" 
 папки могут иметь в имени- только цыфры, от 01 до 255.
 названия треков имеет 3 цыфры впереди
в данном скетче используется следующий порядок папок и треков
папка 01 звуки режима 1
папка 02 звуки режима 2
/-/-/-/-/--
папка 05 звуки режима 5

 в папки 01-05  кидаем треки 001ххх.mp3, 002ххх.mp3, 003хх.mp3, 004блабла.mp3, 005ххх.mp3, 007ххх.mp3
 название мелодий можно не удалять -только добавить вначале порядковый номер из трех цифр 
    тут могут быть незначельые отличия в количестве писков сирены и их значения
    001-это 1 писк сирены(постановка на охрану)
    002-это 2 писк сирены(снятие с охраны)
    003-это 3 писка сирены(слабый удар)
    004-это 4 писка сирены(неисправна зона)
    005-это 5 писка сирены(поиск машины)
    007- непрерывно играет сирена (тревога)
 папка "08" это папка с мелодиями для пульта, в ней 001-009 это треки с мелодиями,
 папка "08" треки 010-015 назкание режимов. 010 - случайный перебор.
     например :
     011-проговаривает "один"
     012-проговаривает "два"...
     015-проговаривает "пять"
 */
 
#include <avr/sleep.h>
#include <SoftwareSerial.h>//вкл альтернативный вход RX TX//для будущего проекта
SoftwareSerial mySoftwareSerial(6, 7);  // RX, TX
#include <DFRobotDFPlayerMini.h>
DFRobotDFPlayerMini myDFPlayer;

unsigned long  SEREN_time=0;     // таймер работы сирены 
uint8_t  ZonaSeren=1, TrekIR=0, randNumber=0 ; //
 bool Seren_activity=0;      // флаг cирены //
volatile bool FLG_Sleep =1;  // тут "1" для того, чтобы ардуино уснул после стартового трека,а не раньше :)
//------------------------настройки-----------------------------
// раскомментируйте ниже строку, если хотите увидеть коды пульта.
#define DEBUG_TO_COMPORT    // печатаем в порт 

uint8_t Regim=0;             // (0~5) можем задать номер режима -номер папки с треками//"0" это случайный выбор по всем папкам
#define SEREN 2              //№ пина вход сирены
//#define RECVR 3              //№ пина вход пульта
#define BUSY  9              //№ пина для контроля плеера
#define Power   8            //№ пина включ унч  и плеер
#define sbros_time 700       //время между писками сирены (время должно быть чуть больше пауз) 
#define max_regim    5       //максимальное количество режимов (количество  папке)
#define volum       30       //громкость
#define ALARM7       7       // это № трекa с мелодиями непрерывного звучания (в каждой папке есть)
// тут можно поменять мелодию при первом включении системы//например
#define papkaN      3        // 3-это папка с мелодией старта
#define trek_start  1        // 1-это номер трека  в папке  


void myISR () {FLG_Sleep  = 1;} // не разрешаем снова уснуть пока флаг  =1

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


if (!FLG_Sleep )
 { //если все условия выше отработали свои задачи (флаги=0)- разрешам спать 
  attachInterrupt(digitalPinToInterrupt(SEREN),myISR, HIGH); // Прерывание по высокому уровню
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);          //Устанавливаем  режим событие но высокому уровню
  sleep_mode();                                 // Переводим МК в спящий режим
  detachInterrupt(digitalPinToInterrupt(SEREN));// Выключаем обработку внешнего прерывания и возвращаемся в то место где уснули
 }//*/ 
}

void OF_Power (){
  TrekIR=0; ZonaSeren=0;     // cброс счетчиков
  digitalWrite(Power, LOW);  // выкл питание плеера и УНЧ
  delay(50);
  FLG_Sleep = 0;  // спать
 }
void payza(unsigned long _mls){//замена 
    unsigned long _timeout = millis() + _mls; 
    while ( millis() < _timeout)  {};  //пустой цикл . ждем не более time_mls
   }
//КОНЕЦ , А КТО ЧИТАЛ МОЛОДЕЦ  
