// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       RemoteControlGarageDoors.ino
    Created:	24.03.2019 14:14:20
    Author:     Aliaksandr Hubkin
*/

// Описание HTTP заголовков - https://code.tutsplus.com/ru/tutorials/http-headers-for-dummies--net-8039. 
// Конвертер base64 - http://base64.ru/.
// Пример  Web Server LED Control из библиотеки W5200.
// Еще один пример - http://wikihandbk.com/wiki/Arduino:%D0%9F%D1%80%D0%B8%D0%BC%D0%B5%D1%80%D1%8B/%D0%92%D0%B5%D0%B1-%D1%81%D0%B5%D1%80%D0%B2%D0%B5%D1%80_%D0%B8%D0%B7_%D0%BF%D0%BB%D0%B0%D1%82%D1%8B_Arduino_%D0%B8_%D1%88%D0%B8%D0%BB%D0%B4%D0%B0_Arduino_Ethernet,_%D1%83%D0%BF%D1%80%D0%B0%D0%B2%D0%BB%D1%8F%D1%8E%D1%89%D0%B8%D0%B9_%D1%80%D0%B5%D0%BB%D0%B5.

// Библиотеки.
#include <SPI.h>
#include <EthernetV2_0.h>

// Включена ли отладка.
//#define UARTDebug

// Логин и пароль в формате Base64 (admin:ArburgDoor19).
#define LoginPassword        "YWRtaW46QXJidXJnRG9vcjE5"

// MAC-адрес ардуины.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// IP-адрес ардуины.
IPAddress ip(192, 168, 0, 77);

// Инициализируем библиотеку Ethernet Server, указывая нужный порт.
EthernetServer server(80);

// Массивы relayPins, relayState и relayName, должны содержать одинаковое кол-во элементов массива.
// Указываем номера пинов для реле.
const byte relayPins[] = { 2, 3, 7, 8 };
// Указываем начальное состояние пинов.
byte relayState[] = { 1, 0, 0, 0 };
// Указываем пояснения к пинам реле. Они будут отображаться на странице сайта.
const char* relayName[] = { "Защита двери", "Открытие двери", "Освещение", "Камера" };

// Задаем переменные для хранения HTTP-запроса.
// Массив для хранения строки HTTP-запроса.
char lineBuf[80];
// Счетчик для массива lineBuf.
byte charCount = 0;

// Записываем в ответ HTTP-сервера, HTTP заголовок Content-Type.
void WriteHttpHeaderContentType(EthernetClient &client)
{
	client.println(F("Content-Type: text/html"));
}

// Записываем в ответ HTTP-сервера, HTTP заголовок с статусом 401.
void WriteHttpStatus401AndHeader(EthernetClient &client)
{
	client.println(F("HTTP/1.1 401 Unauthorized"));
	client.println(F("WWW-Authenticate: Basic realm=\"WebServer\""));
	WriteHttpHeaderContentType(client);
	client.println();
}

// Записываем в ответ HTTP-сервера, статус 200 и HTTP заголовоки.
void WriteHttpStatus200AndHeader(EthernetClient &client)
{
	// Ответ HTTP-сервера, с статусом 200.
	client.println(F("HTTP/1.1 200 OK"));
	WriteHttpHeaderContentType(client);
	client.println(F("Connnection: close"));
	client.println();
}

// Записываем в ответ Html страницу.
void WriteHttpPage(EthernetClient &client)
{
	client.print(F("<!DOCTYPE HTML><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />"));
	client.print(F("<meta charset=\"utf-8\" /><title>Удаленное управление роллетами</title></head>"));
	client.println(F("<body><h3>Вэб сервер удаленного управления роллетами <a href=\"/\">(Обновить страницу)</a></h3>"));

	// Отображаем кнопки управления реле.
	for (int i = 0; i < sizeof(relayPins) / sizeof(relayPins[0]); i++)
	{
		client.print(F("<h4>"));
		client.print(relayName[i]);
		if (relayState[i] == 0)
		{
			// Если реле выключено.
			client.print(F(" Текущее состояние: Выключено</h4>"));
		}
		else
		{
			// Если реле включено.
			client.print(F(" Текущее состояние: Включено</h4>"));
		}

		client.print(F("<a href=\"/pin"));
		client.print((i + 1));

		if (relayState[i] == 0)
		{
			// Если реле выключено.
			client.println(F("On\"><button>Включить</button></a>"));
		}
		else
		{
			// Если реле включено.
			client.println(F("Off\"><button>Выключить</button></a>"));
		}
	}

	client.println(F("</body></html>"));
}

// The setup() function runs once each time the micro-controller starts
// Функция setup запускает только один раз, после каждой подачи питания или сброса платы Arduino.
void setup()
{
	// Настраиваем пины для работы с реле.
	for (int i = 0; i < sizeof(relayPins) / sizeof(relayPins[0]); i++)
	{
		// Устанавливаем пин в режим работы выход.
		pinMode(relayPins[i], OUTPUT);

		// Задаем начальное значение пина.
		digitalWrite(relayPins[i], relayState[i]);
	}

#if defined(UARTDebug)
	// Открываем последовательную коммуникацию на скорости 9600 бод.
	Serial.begin(9600);
	while (!Serial)
	{
		; // Wait for serial port to connect. Needed for native USB port only.
	}
#endif


	// Запускаем Ethernet-коммуникацию и сервер.
	Ethernet.begin(mac, ip);
	server.begin();

	// Делаем небольшую задержку, чтобы все успело запуститься.
	delay(1000);
}


// Add the main program code into the continuous loop() function
// Основной цикл.
void loop() {
	// Прослушиваем входящих клиентов.
	// Если клиент, подключенный к серверу, имеет непрочитанные данные, то функция возвращает описывающий его объект Client.
	EthernetClient client = server.available();

	if (client)
	{
		// Новый клиент.

#if defined(UARTDebug)
	// Отправляем отладочную информацию, "Новый клиент".
		Serial.println(F("New client"));
#endif

		// Заполняем массив 0-ми. Обнуляем массив lineBuf.
		memset(lineBuf, 0, sizeof(lineBuf));

		// Обнуляем счетчик для массива lineBuf.
		charCount = 0;

		// Прошли ли авторизацию.
		bool isAuthorization = false;

		// Является ли текущая строка пустой.
		boolean currentLineIsBlank = true;

		// HTTP-запрос, состоит из линии запроса и заголовков.
		// Линия запроса состоит из метода, пути и протокола. Идет первой строкой в HTTP-запросе.
		// HTTP-заголовки идут следом за линией запроса, каждый заголовок оформлен отдельной строкой и выглядит как пара "Имя: Значение".
		// HTTP-запрос заканчивается пустой строкой.

		// Проверяет, подключен ли клиент или нет.
		while (client.connected())
		{
			// Есть ли непрочитанные байты.
			if (client.available())
			{
				// Сохраняем текущий байт.
				char c = client.read();

#if defined(UARTDebug)
				// Отправляем отладочную информацию, печатаем HTTP-запрос.
				Serial.write(c);
#endif

				// Считываем HTTP-запрос, символ за символом и записываем текущую строку в массив lineBuf.
				lineBuf[charCount] = c;

				// Увеличиваем счетчик для массива lineBuf.
				if (charCount < sizeof(lineBuf) - 1)
				{
					charCount++;
				}

				// Если мы дошли до пустой строки (т.е. если получили символ новой строки в начале строки),
				// это значит, что HTTP-запрос завершен, и мы можем отправить ответ.
				// Если текущий символ конец строки и текущая строка пустая.
				if (c == '\n' && currentLineIsBlank)
				{
					// Если авторизация не пройдена, записываем ответ со статусом 401.
					if (!isAuthorization)
					{
						// Записываем в ответ HTTP-сервера статус 401 и HTTP заголовки.
						WriteHttpStatus401AndHeader(client);
					}
					else
					{
						// Записываем в ответ HTTP-сервера, статус 200 и HTTP заголовоки.
						WriteHttpStatus200AndHeader(client);
						// Записываем в ответ Html страницу.
						WriteHttpPage(client);
					}

					// Выход из цикла.
					break;
				}

				// Если текущий символ конец строки.
				if (c == '\n')
				{
					// Если текущая строка содержит пару, логин:пароль в формате Base64.
					// http://base64.ru/.
					if (strstr(lineBuf, LoginPassword) > 0)
					{
						// Прошли авторизацию.
						isAuthorization = true;
					}

					// Если текущая строка содержит "GET /pin".
					if (strstr(lineBuf, "GET /pin") > 0)
					{
						// Определяем номер пина.
						// В основу определения лежит идея, что используется метод GET и ссылка имеет формат /pin +
						// номер пина в массиве relayPins (от 0 до 99) + On или Off (включен или выключен).

						// Получаем 1 цифру пина.
						byte pinNumber = (lineBuf[8] - 48);

						// Получаем 2 цифру пина.
						byte secondNumber = (lineBuf[9] - 48);

						// Если secondNumber является цифрой.
						if (secondNumber >= 0 && secondNumber <= 9)
						{
							// Высчитываем номер пина.
							pinNumber *= 10;
							pinNumber += secondNumber;
						}

						// Получаем индекс в массиве, по номеру элемента в массиве.
						byte arrayIndex = pinNumber - 1;

						// Переменная для хранения признака нового состояния пина.
						// Если offOrOn == 'n', то включаем.
						// Если offOrOn == 'f', то выключаем.
						char offOrOn;

						// Если номер пина в массиве меньше 10.
						if (pinNumber < 10)
						{
							offOrOn = lineBuf[10];
						}
						else
						{
							offOrOn = lineBuf[11];
						}

						// Новое состояние пина.
						byte relayNewState = 0;
						if (offOrOn == 'n')
						{
							relayNewState = 1;
						}

						// Записываем новое состояние пина и его состояние в массив relayState.
						digitalWrite(relayPins[arrayIndex], relayNewState);
						relayState[arrayIndex] = relayNewState;
					}

					// Если получили символ новой строки.
					currentLineIsBlank = true;

					// Заполняем массив 0-ми. Обнуляем массив lineBuf.
					memset(lineBuf, 0, sizeof(lineBuf));

					// Обнуляем счетчик для массива lineBuf.
					charCount = 0;
				}
				else if (c != '\r')
				{
					// Если не символ возврата коретки.

					// Если получили какой-то другой символ.
					currentLineIsBlank = false;
				}
			}
		}

		// Даем веб-браузеру время на получение данных.
		delay(1);

		// Закрываем соединение.
		client.stop();

#if defined(UARTDebug)
		// Отправляем отладочную информацию, "Клиент отключен".
		Serial.println(F("Client disonnected"));
#endif
	}
}
