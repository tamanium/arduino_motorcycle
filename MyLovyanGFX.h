
#include <LovyanGFX.hpp>
class LGFX : public lgfx::LGFX_Device{

	// 接続するパネルの型にあったインスタンスを用意します。
	lgfx::Panel_ST7789  _panel_instance;
	// パネルを接続するバスの種類にあったインスタンスを用意します。
	lgfx::Bus_SPI       _bus_instance;   // SPIバスのインスタンス
	// バックライト制御が可能な場合はインスタンスを用意します。(必要なければ削除)
	lgfx::Light_PWM     _light_instance;

	public:LGFX(void){
		{ // バス制御の設定を行います。
			auto cfg = _bus_instance.config();    // バス設定用の構造体を取得します。
			// SPIバスの設定
			cfg.spi_host = 0;				// 使用するSPIを選択
			cfg.spi_mode = 0;				// SPI通信モードを設定 (0 ~ 3)
			cfg.freq_write = 40000000;	// 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
			cfg.freq_read = 20000000;		// 受信時のSPIクロック
			cfg.pin_sclk = 2;				// SPIのSCLKピン番号を設定
			cfg.pin_mosi = 3;				// SPIのMOSIピン番号を設定
			cfg.pin_miso = -1;			// SPIのMISOピン番号を設定 (-1 = disable)
			cfg.pin_dc = 7;				// SPIのD/Cピン番号を設定  (-1 = disable
			_bus_instance.config(cfg);	// 設定値をバスに反映します。
			_panel_instance.setBus(&_bus_instance);	// バスをパネルにセットします。
		}

		{ // 表示パネル制御の設定を行います。
			auto cfg = _panel_instance.config();	// 表示パネル設定用の構造体を取得します。
			cfg.pin_cs = 6;			// CSが接続されているピン番号   (-1 = disable)
			cfg.pin_rst = 8;			// RSTが接続されているピン番号  (-1 = disable)
			cfg.pin_busy = -1;		// BUSYが接続されているピン番号 (-1 = disable)
			cfg.panel_width = 240;	// 実際に表示可能な幅
			cfg.panel_height = 320;	// 実際に表示可能な高さ
			cfg.offset_rotation = 1;	// 回転方向の値のオフセット 0~7 (4~7は上下反転)
			cfg.invert = true;		// パネルの明暗が反転してしまう場合 trueに設定
			_panel_instance.config(cfg);
		}

		{ // バックライト制御の設定を行います。（必要なければ削除）
			auto cfg = _light_instance.config();    // バックライト設定用の構造体を取得します。
			cfg.pin_bl = 5;				// バックライトが接続されているピン番号
			cfg.invert = false;			// バックライトの輝度を反転させる場合 true
			cfg.freq   = 44100;			// バックライトのPWM周波数
			cfg.pwm_channel = 7;			// 使用するPWMのチャンネル番号
			_light_instance.config(cfg);
			_panel_instance.setLight(&_light_instance);	// バックライトをパネルにセットします。
		}
		setPanel(&_panel_instance);		// 使用するパネルをセットします。
	}
};
