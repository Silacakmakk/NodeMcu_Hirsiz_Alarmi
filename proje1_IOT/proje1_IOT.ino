// shiftr.io sitesindeki projemize veri gönderme
// gönderilen konu: hirsiz_alarmi, gönderilen veri:HAreket algılandı mi?

#include <ESP8266WiFi.h>
#include <MQTTClient.h>
#include <FirebaseArduino.h>

//*************************************************************************   
#define FIREBASE_HOST "hirsiz-9e48b-default-rtdb.firebaseio.com"     // veritabanının linki
#define FIREBASE_AUTH "JxewKszCtkTX7ECAwoGrM5LRfcn3QuZgVTeIeIkB"              // veritabanı secret key

//*************************************************************************

//burada degerleri degişmeyecek degişkenlerimizi tanımlıyorum
const int buton1=D0;
const int buton2=D1;
const int pir_sensor=D2; 
const int buzzer=D3;

int uyari=0;//nodemcuya ilk enerji verildiğinde buzzerimiz devrede olmayacaktir

const char *agin_adi= "Superbox_WiFi_E3C8";                 // bağlanılacak olan kablosuz ağın agin adi si
const char *sifre = "14+8=Yirmi";                  // bağlanılacak olan kablosuz ağın şifresi

WiFiClient wifi_istemci;
MQTTClient mqtt_istemci;

//**************************************************************************************************************

void setup() 
{
  pinMode(buton1, INPUT);      // D0 pinini giriş olarak ayarla
  pinMode(buton2, INPUT);      // D1 pinini giriş olarak ayarla  
  pinMode(pir_sensor, INPUT);      // D2 pinini giriş olarak ayarla 
  pinMode(buzzer, OUTPUT);      // D3 pinini cikis olarak ayarla 
 
  Serial.begin(9600);                             //Seri iletişim hızı 9600 bps olarak ayarlanarak başlatılıyor.
  delay(1000);
  Serial.println("Wifi agina baglaniliyor");

  WiFi.mode(WIFI_STA);                           //ESP8266 istasyon moduna alınıyor. 
  WiFi.begin(agin_adi, sifre);                    //agin adi ve şifre girilierek wifi başlatılıyor.

  //***********Wifi ağına bağlanıncaya kadar beklenilen kısım **************
  
  while (WiFi.status() != WL_CONNECTED) 
  { 
    delay(500);                                 // Bu arada her 500 ms de bir seri ekrana yan yana noktalar yazdırılarak
    Serial.print(".");                          // görsellik sağlanıyor.
  }
  
  //*******************************************************************************************************************

  Serial.println("");                           //Bir alt satıra geçiliyor.
  Serial.println("Bağlantı sağlandı...");       //seri ekrana bağlantının kurulduğu bilgisi gönderiliyor.

  Serial.print("Alınan IP addresi: ");          // kablosuz ağdan alınan IP adresi
  Serial.println(WiFi.localIP());               // kablosuz ağdan alınan IP adresi
 
  mqtt_istemci.begin("hirsiz-alarmi.cloud.shiftr.io", wifi_istemci); // site üzerindeki projeye linkine bağlan
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);  //firebase baslatiliyor
}

//*****************************************************************************************************************

void loop() 
{
  mqtt_istemci.loop();              // broker a veri gönderebilmek ve gelen veriyi okuyabilmek için******
  
  if (!mqtt_istemci.connected())        // broker a bağlanmadı mı?
    baglan();                 // bağlı değil baglan fonkisyonuna git
    
  hirsiz_alarm();
 
  //delay(3000);                    // 3sn bekle
}

//***************************************************************************************************************

void baglan()
{
  // baglanana kadar bekle ekrana . yazdır
  while (!mqtt_istemci.connect("202113171078", "hirsiz-alarmi", "GsL50YZiUIAel1nx")) // birinci boşluk=sizin client adınız,2.boşluk proje ismi=proje adı, 3.boşluk= token
  {
    Serial.print(".");
    delay(1000);
  }
}
//****************************************************************************************************************

void hirsiz_alarm()
{
  if(digitalRead(buton1)==HIGH)       //Buton bire basıldiysa devre aktif olacaktir
  {
    uyari=1;
    delay(5000); /*uyaimiz aktif olacak ancak 5 sn bekleyeyip(o alandan ayrilma süresi)
    böylece uyarimizin yani buzzer devre elemanının boşa çalışmasini önlemiş olacagiz.*/
    Serial.println("basladi");
  }
  if(digitalRead(buton2)==HIGH)       
  {
    uyari=0;         //Burada herhangi bir gecikmeye ihtiyacimiz bulunmamaktadir.
  }
  
  Serial.print("Uyari sistemimiz:");
  Serial.println(uyari);             //uyari sistemimizin aktif mi aktif degil mi olma durumunu seri ekranda görmek için serial print ve println kullandım.
 
  if(uyari==1)                   //eger uyari sistemimiz aktif olmuşsa hareket var mi diye kontrol ediyoruz
  {
    if(digitalRead(pir_sensor)==HIGH)  //hareket algılandıysa
    { 
      Firebase.setInt("Hareket Algilandi", uyari); //uyari true ise haraket aldilandi çiktisi verecek degil ise bir cikti vermeyecek
    
      while(digitalRead(buton2)== LOW)  //dugme ikiye basılmadiği sürece while dongümüze girer
      {   
      mqtt_istemci.publish("hirsiz_alarmi", "Hareket algilandi");
      digitalWrite(buzzer,HIGH);  //sesli sinyaller üretiyoruz(alarm aktif oldugu sürece sesli sinyaller üretir).
      delay(500);   //buzzerimiz 500 ms saniye sesli sinyal üretecek
      digitalWrite(buzzer,LOW);  //çalip susma şeklinde 
      delay(500);   //buzzerimiz her 500 ms de sesli sinyal üretip ver ardından 500 ms saniye üretip tekrar durucak
      }
    }
  }
 else {
      //5 saniyede bir firebase haraket algilanmadi yazar
      static unsigned long oncekiMillis = 0;
      const long aralik = 5000; // 5 saniye
      /* unsigned long :bir tamsayı veri türüdür. Bu tür, 0'dan başlayarak yüksek sayısal değerlere
      kadar pozitif tamsayıları temsil etmek için kullanılır. 
      unsigned kelimesi, bu türün sadece pozitif değerleri alabileceğini belirtir, yani negatif değerler içermez.*/

      unsigned long suankiMillis = millis();//o andaki saniyeyi verir

      if (suankiMillis - oncekiMillis >= aralik) {
        oncekiMillis = suankiMillis;
        Firebase.setInt("Hareket Algilanmadi", uyari);}
      //tek satırda if blogu
}}
