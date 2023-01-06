#include <unistd.h>

#include <iostream>
#include <MQTTClient.h>
#include <mqtt/client.h>


int main(int argc, char **argv)
{
  const std::vector<std::string> TOPICS{"data/#", "CommandTopic"};
    const std::vector<int>         QOS{0, 1};

  mqtt::client cli("tcp://10.60.4.151:1883", "async_publish");

  mqtt::connect_response rsp = cli.connect();
  if (!rsp.is_session_present())
  {
    cli.subscribe(TOPICS, QOS);
    std::cout << "mqtt_listening: Subscribing to topics OK1" << std::endl;
  }
  else
  {
    std::cout << "already!\n";
  }

  sleep(5);


  return 0;
}
