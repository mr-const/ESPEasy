#if 1 //def USES_P223
//#######################################################################################################
//######################## Plugin 223: Audio Track Player using MAX98357A I2S DAC #######################
//#######################################################################################################

#include "_Plugin_Helper.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"


#define PLUGIN_223
#define PLUGIN_ID_223         223
#define PLUGIN_NAME_223       "AudioPlayer"
#define PLUGIN_VALUENAME1_223 "PlayTime"

#define P223_MP3    1

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2S *out;

class AudioLogRedirect : public Print
{
  virtual size_t write(uint8_t character) override
  {
    process_serialWriteBuffer(); // Try to make some room first.
    int roomLeft = getMaxFreeBlock();

    if (roomLeft < 1000)
    {
      roomLeft = 0; // Do not append to buffer.
    }
    else if (roomLeft < 4000)
    {
      roomLeft = 128 - serialWriteBuffer.size(); // 1 buffer.
    }
    else
    {
      roomLeft -= 4000; // leave some free for normal use.
    }

    if (roomLeft > 0)
    {
      serialWriteBuffer.push_back(character);
      return 1;
    }

    return 0;
  }

  void addToSerialBuffer(const uint8_t *buffer, size_t line_length) {
  process_serialWriteBuffer(); // Try to make some room first.
  int roomLeft             = getMaxFreeBlock();

  if (roomLeft < 1000) {
    roomLeft = 0;                              // Do not append to buffer.
  } else if (roomLeft < 4000) {
    roomLeft = 128 - serialWriteBuffer.size(); // 1 buffer.
  } else {
    roomLeft -= 4000;                          // leave some free for normal use.
  }

  if (roomLeft > 0) {
    size_t pos = 0;

    while (pos < line_length && pos < static_cast<size_t>(roomLeft)) {
      serialWriteBuffer.push_back(buffer[pos]);
      ++pos;
    }
  }
}

  virtual size_t write(const uint8_t *buffer, size_t size) override {
    ::addToSerialBuffer(String(millis()).c_str());
    ::addToSerialBuffer(" : ");
    String loglevelDisplayString = "AUDIO";
    while (loglevelDisplayString.length() < 6) {
      loglevelDisplayString += ' ';
    }
    ::addToSerialBuffer(loglevelDisplayString.c_str());
    ::addToSerialBuffer(": ");
    addToSerialBuffer(buffer, size);
    addNewlineToSerialBuffer();
  }
};

AudioLogRedirect logger;

void mp3StatusCB(void *cbData, int code, const char *string)
{
  addLog(LOG_LEVEL_ERROR, String(F("MP3 Status: ")) + string);
}

boolean Plugin_223(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_223;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].VType = SENSOR_TYPE_TEMP_HUM;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_223);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_223));
        break;
      }

    case PLUGIN_GET_DEVICEGPIONAMES:
      {
        event->String1 = formatGpioName_bidirectional(F("Data"));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        const String options[] = { F("MP3") };
        int indices[] = { P223_MP3 };

        addFormSelector(F("Sensor model"), F("p223_audio"), 5, options, indices, PCONFIG(0) );

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        PCONFIG(0) = getFormItemInt(F("p223_audio"));

        success = true;
        break;
      }

    case PLUGIN_INIT:
    {
        addLog(LOG_LEVEL_INFO, F("Initializing AudioPlayer"));
        audioLogger = &logger;

        bool rc = SPIFFS.begin();
        if (!rc)
          addLog(LOG_LEVEL_ERROR, F("Failed to initialize SPIFFS"));

        file = new AudioFileSourceSPIFFS("track.mp3");
        
        if (!file->isOpen())
        {
          addLog(LOG_LEVEL_ERROR, F("Failed to open MP3 file"));
        }
        else
        {
          addLog(LOG_LEVEL_INFO, F("Successfully opened MP3 File"));
        }
        
        out = new AudioOutputI2S();
        mp3 = new AudioGeneratorMP3();
        mp3->RegisterStatusCB(&mp3StatusCB, nullptr);

        rc = mp3->begin(file, out);

        if (!rc)
        {
          addLog(LOG_LEVEL_ERROR, F("MP3 Playback failed"));
        }
        else
        {
          addLog(LOG_LEVEL_INFO, F("Started Playback"));
        }

        success = true;
        break;
    }

    case PLUGIN_ONCE_A_SECOND:
    {
        if (mp3->isRunning()) 
        {
          if (!mp3->loop()) mp3->stop();
          addLog(LOG_LEVEL_INFO, F("MP3 Tick"));
        }
        else
        {
          addLog(LOG_LEVEL_INFO, F("Playback completed"));
        }
        success = true;
      break;
    }

    case PLUGIN_READ:
      {

        break;
      }
  }
  return success;
}

#endif