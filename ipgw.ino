#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <string>
#include <cstdio>

const String USERNAME = "YOUR_ID"; // 校园网用户名
const String PASSWORD = "YOUR_PASSWORD"; // 校园网密码

ESP8266WiFiMulti WiFiMulti;

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // 添加可连接的WiFi SSID和密码
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP("NEU-2.4G", "");
    WiFiMulti.addAP("NEU", "");

    Serial.println();
    Serial.print("Wait for WiFi... ");

    while (WiFiMulti.run() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);

    WiFiClient client;
    
    // 在ipgw之前，先进行一次公网请求
    Serial.println("#0 connect 198.18.0.1 ");
    if (!client.connect("198.18.0.1", 80))
    {
        Serial.println("connection failed");
    }

    // 首先需要获取ac_id，不同网络环境下ac_id不同
    Serial.println("#1 connect ipgw to get ac_id");
    if (!client.connect("ipgw.neu.edu.cn", 80))
    {
        Serial.println("connection failed");
        return;
    }
    client.print("GET /index_1.html HTTP/1.1\r\n");
    client.print("Host: ipgw.neu.edu.cn\r\n");
    client.print("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n");
    client.print("\r\n");
    Serial.println("receiving from remote server by GET ipgw ac_id");
    String re = client.readString();
    char *ac_id = strstr(re.c_str(), "ac_id=");
    String ac_id_s = "";
    size_t cnt = 0;
    for (size_t i = 0; i < strlen(ac_id); i++)
    {
        if (ac_id[i] == '&')
        {
            break;
        }
        if (i != 0 && ac_id[i - 1] == '=')
        {
            ac_id_s += ac_id[i];
            cnt = 1;
            continue;
        }
        if (cnt == 1)
        {
            ac_id_s += ac_id[i];
        }
    }
    Serial.println("ac_id: " + ac_id_s);
    client.stop();
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println("#2 connect SSO to get LT");
    if (!client.connect("pass.neu.edu.cn", 80))
    {
        Serial.println("connection failed");
        Serial.println("wait 5 sec...");
        delay(5000);
        return;
    }
    client.print("GET /tpass/login?service=http%3A%2F%2Fipgw.neu.edu.cn%2Fsrun_portal_sso%3Fac_id%3D" + ac_id_s + " HTTP/1.1\r\n");
    client.print("Host: pass.neu.edu.cn\r\n");
    client.print("Connection: keep-alive\r\n");
    client.print("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/99.0.4844.74 Safari/537.36 Edg/99.0.1150.46\r\n");
    client.print("\r\n");
    String line = client.readString();
    char *lt = strstr(line.c_str(), "lt");
    cnt = 0;
    String lt_s;
    for (size_t i = 0; i < strlen(lt); i++)
    {
        if (cnt == 4 && lt[i] != '\"')
        {
            lt_s += lt[i];
        }
        if (cnt == 5)
        {
            break;
        }
        if (lt[i] == '\"')
        {
            cnt++;
        }
    }

    char *lp = strstr(line.c_str(), "id=\"loginForm\" action=\"");
    // 需要把loginPath获取到
    String lp_s;
    cnt = 0;
    for (size_t i = 0; i < strlen(lp); i++)
    {
        if (cnt == 3 && lp[i] != '\"')
        {
            lp_s += lp[i];
        }
        if (cnt == 4)
        {
            break;
        }
        if (lp[i] == '\"')
        {
            cnt++;
        }
    }
    char ul[10];
    char pl[20];
    sprintf(ul, "%d", USERNAME.length());
    sprintf(pl, "%d", PASSWORD.length());
    String login_body = "rsa=" + USERNAME + PASSWORD + lt_s + "&ul=" + ul + "&pl=" + pl + "&lt=" + lt_s +
                        "&execution=e1s1&_eventId=submit";

    Serial.println("payload is ready");
    client.stop();

    char *cookie = strstr(lp_s.c_str(), "jsessionid_tpass=");
    String cookie_s;
    cnt = 0;
    for (size_t i = 0; i < strlen(cookie); i++)
    {
        if (cookie[i] == '=')
        {
            cnt = 1;
            continue;
        }
        if (cnt == 1 && cookie[i] != '?')
        {
            cookie_s += cookie[i];
        }
        if (cookie[i] == '?')
        {
            break;
        }
    }
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println("#3 POST payload to get the ticket");
    if (!client.connect("pass.neu.edu.cn", 80))
    {
        Serial.println("connection failed");
        return;
    }
    char body_len[10];
    sprintf(body_len, "%d", login_body.length());
    String body_len_s = String(body_len);

    client.print("POST " + lp_s + " HTTP/1.1\r\n");
    client.print("Content-Type: application/x-www-form-urlencoded\r\n");
    client.print("Host: pass.neu.edu.cn\r\n");
    client.print("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n");
    client.print("Origin: http://pass.neu.edu.cn\r\n");
    client.print("Connection: keep-alive\r\n");
    client.print("Cookie: Language=zh_CN; jsessionid_tpass=" + cookie_s + "\r\n");
    client.print("Content-Length: " + body_len_s + "\r\n");
    client.print("\r\n");
    client.print(login_body);

    Serial.println("receiving from remote server by POST");
    line = client.readString();
    Serial.printf("line len: %d\n", line.length());
    client.stop();

    char *location = strstr(line.c_str(), "Location: ");
    cnt = 0;
    String location_s;
    size_t len = strlen(location);
    for (size_t i = 0; i < len; i++)
    {
        if (location[i] == '\r' || location[i] == '\n')
        {
            break;
        }
        if (location[i] == '/')
        {
            cnt++;
        }
        if (cnt == 3)
        {
            location_s += location[i];
        }

    }

    Serial.println(location_s);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println("#4 use ticket to login ipgw");
    if (!client.connect("ipgw.neu.edu.cn", 80))
    {
        Serial.println("connection failed");
        return;
    }
    char *ticket = strstr(location_s.c_str(), "ticket=");
    Serial.println("GET /v1/srun_portal_sso?ac_id=" + ac_id_s + ";" + ticket);
    client.print("GET /v1/srun_portal_sso?ac_id=" + ac_id_s + ";" + ticket + " HTTP/1.1\r\n");
    client.print("Host: ipgw.neu.edu.cn\r\n");
    client.print("Connection: keep-alive\r\n");
    client.print("Accept: */*\r\n");
    client.print("Accept-Encoding: gzip, deflate\r\n");
    client.print("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n");
    client.print("\r\n");
    Serial.println("receiving from remote server by GET ipgw v1");
    line = client.readString();
    Serial.println(line);
    client.stop();

    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println("setup end");
}

void loop()
{
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
}