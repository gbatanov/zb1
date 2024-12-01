#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "ssd1306.h"
#include "font8x8_basic.h"

#define TAG "SSD1306"
#define I2C_TICKS_TO_WAIT 100	  // Maximum ticks to wait before issuing a timeout.

#define PACK8 __attribute__((aligned(__alignof__(uint8_t)), packed))

typedef union out_column_t
{
	uint32_t u32;
	uint8_t u8[4];
} PACK8 out_column_t;

// Инициализация дисплея
void ssd1306_init(SSD1306_t *dev, int width, int height)
{

	dev->_width = width;
	dev->_height = height;
	dev->_pages = 8;
	if (dev->_height == 32)
		dev->_pages = 4;

	uint8_t out_buf[27];
	int out_index = 0;
	out_buf[out_index++] = OLED_CONTROL_BYTE_CMD_STREAM;
	out_buf[out_index++] = OLED_CMD_DISPLAY_OFF;   // AE
	out_buf[out_index++] = OLED_CMD_SET_MUX_RATIO; // A8
	if (dev->_height == 64)
		out_buf[out_index++] = 0x3F;
	if (dev->_height == 32)
		out_buf[out_index++] = 0x1F;
	out_buf[out_index++] = OLED_CMD_SET_DISPLAY_OFFSET; // D3
	out_buf[out_index++] = 0x00;
	// out_buf[out_index++] = OLED_CONTROL_BYTE_DATA_STREAM;	// 40
	out_buf[out_index++] = OLED_CMD_SET_DISPLAY_START_LINE; // 40
	// out_buf[out_index++] = OLED_CMD_SET_SEGMENT_REMAP;		// A1
	if (dev->_flip)
	{
		out_buf[out_index++] = OLED_CMD_SET_SEGMENT_REMAP_0; // A0
	}
	else
	{
		out_buf[out_index++] = OLED_CMD_SET_SEGMENT_REMAP_1; // A1
	}
	out_buf[out_index++] = OLED_CMD_SET_COM_SCAN_MODE;	 // C8
	out_buf[out_index++] = OLED_CMD_SET_DISPLAY_CLK_DIV; // D5
	out_buf[out_index++] = 0x80;
	out_buf[out_index++] = OLED_CMD_SET_COM_PIN_MAP; // DA
	if (dev->_height == 64)
		out_buf[out_index++] = 0x12;
	if (dev->_height == 32)
		out_buf[out_index++] = 0x02;
	out_buf[out_index++] = OLED_CMD_SET_CONTRAST; // 81
	out_buf[out_index++] = 0xFF;
	out_buf[out_index++] = OLED_CMD_DISPLAY_RAM;	   // A4
	out_buf[out_index++] = OLED_CMD_SET_VCOMH_DESELCT; // DB
	out_buf[out_index++] = 0x40;
	out_buf[out_index++] = OLED_CMD_SET_MEMORY_ADDR_MODE; // 20
	// out_buf[out_index++] = OLED_CMD_SET_HORI_ADDR_MODE;	// 00
	out_buf[out_index++] = OLED_CMD_SET_PAGE_ADDR_MODE; // 02
	// Set Lower Column Start Address for Page Addressing Mode
	out_buf[out_index++] = 0x00;
	// Set Higher Column Start Address for Page Addressing Mode
	out_buf[out_index++] = 0x10;
	out_buf[out_index++] = OLED_CMD_SET_CHARGE_PUMP; // 8D
	out_buf[out_index++] = 0x14;
	out_buf[out_index++] = OLED_CMD_DEACTIVE_SCROLL; // 2E
	out_buf[out_index++] = OLED_CMD_DISPLAY_NORMAL;	 // A6
	out_buf[out_index++] = OLED_CMD_DISPLAY_ON;		 // AF

	esp_err_t res;
	res = i2c_master_transmit(dev->_i2c_dev_handle, out_buf, out_index, I2C_TICKS_TO_WAIT);
	if (res == ESP_OK)
	{
		ESP_LOGI(TAG, "OLED configured successfully");
	}
	else
	{
		ESP_LOGE(TAG, "Could not write to device [0x%02x at %d]: %d (%s)", dev->_address, dev->_i2c_num, res, esp_err_to_name(res));
	}

	// Initialize internal buffer
	for (int i = 0; i < dev->_pages; i++)
	{
		memset(dev->_page[i]._segs, 0, 128);
	}
}

int ssd1306_get_width(SSD1306_t *dev)
{
	return dev->_width;
}

int ssd1306_get_height(SSD1306_t *dev)
{
	return dev->_height;
}

int ssd1306_get_pages(SSD1306_t *dev)
{
	return dev->_pages;
}

void ssd1306_show_buffer(SSD1306_t *dev)
{

	for (int page = 0; page < dev->_pages; page++)
	{
		i2c_display_image(dev, page, 0, dev->_page[page]._segs, dev->_width);
	}
}

void ssd1306_set_buffer(SSD1306_t *dev, uint8_t *buffer)
{
	int index = 0;
	for (int page = 0; page < dev->_pages; page++)
	{
		memcpy(&dev->_page[page]._segs, &buffer[index], 128);
		index = index + 128;
	}
}

void ssd1306_get_buffer(SSD1306_t *dev, uint8_t *buffer)
{
	int index = 0;
	for (int page = 0; page < dev->_pages; page++)
	{
		memcpy(&buffer[index], &dev->_page[page]._segs, 128);
		index = index + 128;
	}
}

void ssd1306_display_image(SSD1306_t *dev, int page, int seg, uint8_t *images, int width)
{
	i2c_display_image(dev, page, seg, images, width);

	// Set to internal buffer
	memcpy(&dev->_page[page]._segs[seg], images, width);
}

void ssd1306_display_text(SSD1306_t *dev, int page, char *text, int text_len, bool invert)
{
	if (page >= dev->_pages)
		return;
	int _text_len = text_len;
	if (_text_len > 16)
		_text_len = 16;

	int seg = 0;
	uint8_t image[8];
	for (int i = 0; i < _text_len; i++)
	{
		memcpy(image, font8x8_basic_tr[(uint8_t)text[i]], 8);
		if (invert)
			ssd1306_invert(image, 8);
		if (dev->_flip)
			ssd1306_flip(image, 8);
		ssd1306_display_image(dev, page, seg, image, 8);
		seg = seg + 8;
	}
}

void ssd1306_display_text_box1(SSD1306_t *dev, int page, int seg, char *text, int box_width, int text_len, bool invert, int delay)
{
	if (page >= dev->_pages)
		return;
	int text_box_pixel = box_width * 8;
	if (seg + text_box_pixel > dev->_width)
		return;

	int _seg = seg;
	uint8_t image[8];
	for (int i = 0; i < box_width; i++)
	{
		memcpy(image, font8x8_basic_tr[(uint8_t)text[i]], 8);
		if (invert)
			ssd1306_invert(image, 8);
		if (dev->_flip)
			ssd1306_flip(image, 8);
		ssd1306_display_image(dev, page, _seg, image, 8);
		_seg = _seg + 8;
	}
	vTaskDelay(delay);

	// Horizontally scroll inside the box
	for (int _text = box_width; _text < text_len; _text++)
	{
		memcpy(image, font8x8_basic_tr[(uint8_t)text[_text]], 8);
		if (invert)
			ssd1306_invert(image, 8);
		if (dev->_flip)
			ssd1306_flip(image, 8);
		for (int _bit = 0; _bit < 8; _bit++)
		{
			for (int _pixel = 0; _pixel < text_box_pixel; _pixel++)
			{
				// ESP_LOGI(TAG, "_text=%d _bit=%d _pixel=%d", _text, _bit, _pixel);
				dev->_page[page]._segs[_pixel + seg] = dev->_page[page]._segs[_pixel + seg + 1];
			}
			dev->_page[page]._segs[seg + text_box_pixel - 1] = image[_bit];
			ssd1306_display_image(dev, page, seg, &dev->_page[page]._segs[seg], text_box_pixel);
			vTaskDelay(delay);
		}
	}
}

void ssd1306_display_text_box2(SSD1306_t *dev, int page, int seg, char *text, int box_width, int text_len, bool invert, int delay)
{
	if (page >= dev->_pages)
		return;
	int text_box_pixel = box_width * 8;
	if (seg + text_box_pixel > dev->_width)
		return;

	int _seg = seg;
	uint8_t image[8];

	// Fill the text box with blanks
	for (int i = 0; i < box_width; i++)
	{
		// memcpy(image, font8x8_basic_tr[(uint8_t)text[i]], 8);
		memcpy(image, font8x8_basic_tr[0x20], 8);
		if (invert)
			ssd1306_invert(image, 8);
		if (dev->_flip)
			ssd1306_flip(image, 8);
		ssd1306_display_image(dev, page, _seg, image, 8);
		_seg = _seg + 8;
	}
	vTaskDelay(delay);

	// Horizontally scroll inside the box
	for (int _text = 0; _text < text_len; _text++)
	{
		memcpy(image, font8x8_basic_tr[(uint8_t)text[_text]], 8);
		if (invert)
			ssd1306_invert(image, 8);
		if (dev->_flip)
			ssd1306_flip(image, 8);
		for (int _bit = 0; _bit < 8; _bit++)
		{
			for (int _pixel = 0; _pixel < text_box_pixel; _pixel++)
			{
				// ESP_LOGI(TAG, "_text=%d _bit=%d _pixel=%d", _text, _bit, _pixel);
				dev->_page[page]._segs[_pixel + seg] = dev->_page[page]._segs[_pixel + seg + 1];
			}
			dev->_page[page]._segs[seg + text_box_pixel - 1] = image[_bit];
			ssd1306_display_image(dev, page, seg, &dev->_page[page]._segs[seg], text_box_pixel);
			vTaskDelay(delay);
		}
	}

	// Horizontally scroll inside the box
	for (int _text = 0; _text < box_width; _text++)
	{
		memcpy(image, font8x8_basic_tr[0x20], 8);
		if (invert)
			ssd1306_invert(image, 8);
		if (dev->_flip)
			ssd1306_flip(image, 8);
		for (int _bit = 0; _bit < 8; _bit++)
		{
			for (int _pixel = 0; _pixel < text_box_pixel; _pixel++)
			{
				// ESP_LOGI(TAG, "_text=%d _bit=%d _pixel=%d", _text, _bit, _pixel);
				dev->_page[page]._segs[_pixel + seg] = dev->_page[page]._segs[_pixel + seg + 1];
			}
			dev->_page[page]._segs[seg + text_box_pixel - 1] = image[_bit];
			ssd1306_display_image(dev, page, seg, &dev->_page[page]._segs[seg], text_box_pixel);
			vTaskDelay(delay);
		}
	}
}

// by Coert Vonk
void ssd1306_display_text_x3(SSD1306_t *dev, int page, char *text, int text_len, bool invert)
{
	if (page >= dev->_pages)
		return;
	int _text_len = text_len;
	if (_text_len > 5)
		_text_len = 5;

	int seg = 0;

	for (int nn = 0; nn < _text_len; nn++)
	{

		uint8_t const *const in_columns = font8x8_basic_tr[(uint8_t)text[nn]];

		// make the character 3x as high
		out_column_t out_columns[8];
		memset(out_columns, 0, sizeof(out_columns));

		for (int xx = 0; xx < 8; xx++)
		{ // for each column (x-direction)

			uint32_t in_bitmask = 0b1;
			uint32_t out_bitmask = 0b111;

			for (int yy = 0; yy < 8; yy++)
			{ // for pixel (y-direction)
				if (in_columns[xx] & in_bitmask)
				{
					out_columns[xx].u32 |= out_bitmask;
				}
				in_bitmask <<= 1;
				out_bitmask <<= 3;
			}
		}

		// render character in 8 column high pieces, making them 3x as wide
		for (int yy = 0; yy < 3; yy++)
		{ // for each group of 8 pixels high (y-direction)

			uint8_t image[24];
			for (int xx = 0; xx < 8; xx++)
			{ // for each column (x-direction)
				image[xx * 3 + 0] =
					image[xx * 3 + 1] =
						image[xx * 3 + 2] = out_columns[xx].u8[yy];
			}
			if (invert)
				ssd1306_invert(image, 24);
			if (dev->_flip)
				ssd1306_flip(image, 24);
			i2c_display_image(dev, page + yy, seg, image, 24);
			memcpy(&dev->_page[page + yy]._segs[seg], image, 24);
		}

		seg = seg + 24;
	}
}

void ssd1306_clear_screen(SSD1306_t *dev, bool invert)
{
	char space[16];
	memset(space, 0x00, sizeof(space));
	for (int page = 0; page < dev->_pages; page++)
	{
		ssd1306_display_text(dev, page, space, sizeof(space), invert);
	}
}

void ssd1306_clear_line(SSD1306_t *dev, int page, bool invert)
{
	char space[16];
	memset(space, 0x00, sizeof(space));
	ssd1306_display_text(dev, page, space, sizeof(space), invert);
}

void ssd1306_contrast(SSD1306_t *dev, int contrast)
{
	
	uint8_t _contrast = contrast;
	if (contrast < 0x0)
		_contrast = 0;
	if (contrast > 0xFF)
		_contrast = 0xFF;

	uint8_t out_buf[3];
	int out_index = 0;
	out_buf[out_index++] = OLED_CONTROL_BYTE_CMD_STREAM; // 00
	out_buf[out_index++] = OLED_CMD_SET_CONTRAST;		 // 81
	out_buf[out_index++] = _contrast;

	esp_err_t res = i2c_master_transmit(dev->_i2c_dev_handle, out_buf, 3, I2C_TICKS_TO_WAIT);
	if (res != ESP_OK)
		ESP_LOGE(TAG, "Could not write to device [0x%02x at %d]: %d (%s)", dev->_address, dev->_i2c_num, res, esp_err_to_name(res));

}

void ssd1306_software_scroll(SSD1306_t *dev, int start, int end)
{
	ESP_LOGD(TAG, "software_scroll start=%d end=%d _pages=%d", start, end, dev->_pages);
	if (start < 0 || end < 0)
	{
		dev->_scEnable = false;
	}
	else if (start >= dev->_pages || end >= dev->_pages)
	{
		dev->_scEnable = false;
	}
	else
	{
		dev->_scEnable = true;
		dev->_scStart = start;
		dev->_scEnd = end;
		dev->_scDirection = 1;
		if (start > end)
			dev->_scDirection = -1;
	}
}

void ssd1306_scroll_text(SSD1306_t *dev, char *text, int text_len, bool invert)
{
	ESP_LOGD(TAG, "dev->_scEnable=%d", dev->_scEnable);
	if (dev->_scEnable == false)
		return;

	void (*func)(SSD1306_t *dev, int page, int seg, uint8_t *images, int width);
	func = i2c_display_image;

	int srcIndex = dev->_scEnd - dev->_scDirection;
	while (1)
	{
		int dstIndex = srcIndex + dev->_scDirection;
		ESP_LOGD(TAG, "srcIndex=%d dstIndex=%d", srcIndex, dstIndex);
		for (int seg = 0; seg < dev->_width; seg++)
		{
			dev->_page[dstIndex]._segs[seg] = dev->_page[srcIndex]._segs[seg];
		}
		(*func)(dev, dstIndex, 0, dev->_page[dstIndex]._segs, sizeof(dev->_page[dstIndex]._segs));
		if (srcIndex == dev->_scStart)
			break;
		srcIndex = srcIndex - dev->_scDirection;
	}

	int _text_len = text_len;
	if (_text_len > 16)
		_text_len = 16;

	ssd1306_display_text(dev, srcIndex, text, text_len, invert);
}

void ssd1306_scroll_clear(SSD1306_t *dev)
{
	ESP_LOGD(TAG, "dev->_scEnable=%d", dev->_scEnable);
	if (dev->_scEnable == false)
		return;

	int srcIndex = dev->_scEnd - dev->_scDirection;
	while (1)
	{
		int dstIndex = srcIndex + dev->_scDirection;
		ESP_LOGD(TAG, "srcIndex=%d dstIndex=%d", srcIndex, dstIndex);
		ssd1306_clear_line(dev, dstIndex, false);
		if (dstIndex == dev->_scStart)
			break;
		srcIndex = srcIndex - dev->_scDirection;
	}
}

void ssd1306_hardware_scroll(SSD1306_t *dev, ssd1306_scroll_type_t scroll)
{

	uint8_t out_buf[11];
	int out_index = 0;
	out_buf[out_index++] = OLED_CONTROL_BYTE_CMD_STREAM; // 00

	if (scroll == SCROLL_RIGHT)
	{
		out_buf[out_index++] = OLED_CMD_HORIZONTAL_RIGHT; // 26
		out_buf[out_index++] = 0x00;					  // Dummy byte
		out_buf[out_index++] = 0x00;					  // Define start page address
		out_buf[out_index++] = 0x07;					  // Frame frequency
		out_buf[out_index++] = 0x07;					  // Define end page address
		out_buf[out_index++] = 0x00;					  //
		out_buf[out_index++] = 0xFF;					  //
		out_buf[out_index++] = OLED_CMD_ACTIVE_SCROLL;	  // 2F
	}

	if (scroll == SCROLL_LEFT)
	{
		out_buf[out_index++] = OLED_CMD_HORIZONTAL_LEFT; // 27
		out_buf[out_index++] = 0x00;					 // Dummy byte
		out_buf[out_index++] = 0x00;					 // Define start page address
		out_buf[out_index++] = 0x07;					 // Frame frequency
		out_buf[out_index++] = 0x07;					 // Define end page address
		out_buf[out_index++] = 0x00;					 //
		out_buf[out_index++] = 0xFF;					 //
		out_buf[out_index++] = OLED_CMD_ACTIVE_SCROLL;	 // 2F
	}

	if (scroll == SCROLL_DOWN)
	{
		out_buf[out_index++] = OLED_CMD_CONTINUOUS_SCROLL; // 29
		out_buf[out_index++] = 0x00;					   // Dummy byte
		out_buf[out_index++] = 0x00;					   // Define start page address
		out_buf[out_index++] = 0x07;					   // Frame frequency
		// out_buf[out_index++] = 0x01; // Define end page address
		out_buf[out_index++] = 0x00; // Define end page address
		out_buf[out_index++] = 0x3F; // Vertical scrolling offset

		out_buf[out_index++] = OLED_CMD_VERTICAL; // A3
		out_buf[out_index++] = 0x00;
		if (dev->_height == 64)
			// out_buf[out_index++] = 0x7F;
			out_buf[out_index++] = 0x40;
		if (dev->_height == 32)
			out_buf[out_index++] = 0x20;
		out_buf[out_index++] = OLED_CMD_ACTIVE_SCROLL; // 2F
	}

	if (scroll == SCROLL_UP)
	{
		out_buf[out_index++] = OLED_CMD_CONTINUOUS_SCROLL; // 29
		out_buf[out_index++] = 0x00;					   // Dummy byte
		out_buf[out_index++] = 0x00;					   // Define start page address
		out_buf[out_index++] = 0x07;					   // Frame frequency
		// out_buf[out_index++] = 0x01; // Define end page address
		out_buf[out_index++] = 0x00; // Define end page address
		out_buf[out_index++] = 0x01; // Vertical scrolling offset

		out_buf[out_index++] = OLED_CMD_VERTICAL; // A3
		out_buf[out_index++] = 0x00;
		if (dev->_height == 64)
			// out_buf[out_index++] = 0x7F;
			out_buf[out_index++] = 0x40;
		if (dev->_height == 32)
			out_buf[out_index++] = 0x20;
		out_buf[out_index++] = OLED_CMD_ACTIVE_SCROLL; // 2F
	}

	if (scroll == SCROLL_STOP)
	{
		out_buf[out_index++] = OLED_CMD_DEACTIVE_SCROLL; // 2E
	}

	esp_err_t res = i2c_master_transmit(dev->_i2c_dev_handle, out_buf, out_index, I2C_TICKS_TO_WAIT);
	if (res != ESP_OK)
		ESP_LOGE(TAG, "Could not write to device [0x%02x at %d]: %d (%s)", dev->_address, dev->_i2c_num, res, esp_err_to_name(res));


}

// delay = 0 : display with no wait
// delay > 0 : display with wait
// delay < 0 : no display
void ssd1306_wrap_arround(SSD1306_t *dev, ssd1306_scroll_type_t scroll, int start, int end, int8_t delay)
{
	if (scroll == SCROLL_RIGHT)
	{
		int _start = start; // 0 to 7
		int _end = end;		// 0 to 7
		if (_end >= dev->_pages)
			_end = dev->_pages - 1;
		uint8_t wk;
		// for (int page=0;page<dev->_pages;page++) {
		for (int page = _start; page <= _end; page++)
		{
			wk = dev->_page[page]._segs[127];
			for (int seg = 127; seg > 0; seg--)
			{
				dev->_page[page]._segs[seg] = dev->_page[page]._segs[seg - 1];
			}
			dev->_page[page]._segs[0] = wk;
		}
	}
	else if (scroll == SCROLL_LEFT)
	{
		int _start = start; // 0 to 7
		int _end = end;		// 0 to 7
		if (_end >= dev->_pages)
			_end = dev->_pages - 1;
		uint8_t wk;
		// for (int page=0;page<dev->_pages;page++) {
		for (int page = _start; page <= _end; page++)
		{
			wk = dev->_page[page]._segs[0];
			for (int seg = 0; seg < 127; seg++)
			{
				dev->_page[page]._segs[seg] = dev->_page[page]._segs[seg + 1];
			}
			dev->_page[page]._segs[127] = wk;
		}
	}
	else if (scroll == SCROLL_UP)
	{
		int _start = start; // 0 to {width-1}
		int _end = end;		// 0 to {width-1}
		if (_end >= dev->_width)
			_end = dev->_width - 1;
		uint8_t wk0;
		uint8_t wk1;
		uint8_t wk2;
		uint8_t save[128];
		// Save pages 0
		for (int seg = 0; seg < 128; seg++)
		{
			save[seg] = dev->_page[0]._segs[seg];
		}
		// Page0 to Page6
		for (int page = 0; page < dev->_pages - 1; page++)
		{
			// for (int seg=0;seg<128;seg++) {
			for (int seg = _start; seg <= _end; seg++)
			{
				wk0 = dev->_page[page]._segs[seg];
				wk1 = dev->_page[page + 1]._segs[seg];
				if (dev->_flip)
					wk0 = ssd1306_rotate_byte(wk0);
				if (dev->_flip)
					wk1 = ssd1306_rotate_byte(wk1);
				if (seg == 0)
				{
					ESP_LOGD(TAG, "b page=%d wk0=%02x wk1=%02x", page, wk0, wk1);
				}
				wk0 = wk0 >> 1;
				wk1 = wk1 & 0x01;
				wk1 = wk1 << 7;
				wk2 = wk0 | wk1;
				if (seg == 0)
				{
					ESP_LOGD(TAG, "a page=%d wk0=%02x wk1=%02x wk2=%02x", page, wk0, wk1, wk2);
				}
				if (dev->_flip)
					wk2 = ssd1306_rotate_byte(wk2);
				dev->_page[page]._segs[seg] = wk2;
			}
		}
		// Page7
		int pages = dev->_pages - 1;
		// for (int seg=0;seg<128;seg++) {
		for (int seg = _start; seg <= _end; seg++)
		{
			wk0 = dev->_page[pages]._segs[seg];
			wk1 = save[seg];
			if (dev->_flip)
				wk0 = ssd1306_rotate_byte(wk0);
			if (dev->_flip)
				wk1 = ssd1306_rotate_byte(wk1);
			wk0 = wk0 >> 1;
			wk1 = wk1 & 0x01;
			wk1 = wk1 << 7;
			wk2 = wk0 | wk1;
			if (dev->_flip)
				wk2 = ssd1306_rotate_byte(wk2);
			dev->_page[pages]._segs[seg] = wk2;
		}
	}
	else if (scroll == SCROLL_DOWN)
	{
		int _start = start; // 0 to {width-1}
		int _end = end;		// 0 to {width-1}
		if (_end >= dev->_width)
			_end = dev->_width - 1;
		uint8_t wk0;
		uint8_t wk1;
		uint8_t wk2;
		uint8_t save[128];
		// Save pages 7
		int pages = dev->_pages - 1;
		for (int seg = 0; seg < 128; seg++)
		{
			save[seg] = dev->_page[pages]._segs[seg];
		}
		// Page7 to Page1
		for (int page = pages; page > 0; page--)
		{
			// for (int seg=0;seg<128;seg++) {
			for (int seg = _start; seg <= _end; seg++)
			{
				wk0 = dev->_page[page]._segs[seg];
				wk1 = dev->_page[page - 1]._segs[seg];
				if (dev->_flip)
					wk0 = ssd1306_rotate_byte(wk0);
				if (dev->_flip)
					wk1 = ssd1306_rotate_byte(wk1);
				if (seg == 0)
				{
					ESP_LOGD(TAG, "b page=%d wk0=%02x wk1=%02x", page, wk0, wk1);
				}
				wk0 = wk0 << 1;
				wk1 = wk1 & 0x80;
				wk1 = wk1 >> 7;
				wk2 = wk0 | wk1;
				if (seg == 0)
				{
					ESP_LOGD(TAG, "a page=%d wk0=%02x wk1=%02x wk2=%02x", page, wk0, wk1, wk2);
				}
				if (dev->_flip)
					wk2 = ssd1306_rotate_byte(wk2);
				dev->_page[page]._segs[seg] = wk2;
			}
		}
		// Page0
		// for (int seg=0;seg<128;seg++) {
		for (int seg = _start; seg <= _end; seg++)
		{
			wk0 = dev->_page[0]._segs[seg];
			wk1 = save[seg];
			if (dev->_flip)
				wk0 = ssd1306_rotate_byte(wk0);
			if (dev->_flip)
				wk1 = ssd1306_rotate_byte(wk1);
			wk0 = wk0 << 1;
			wk1 = wk1 & 0x80;
			wk1 = wk1 >> 7;
			wk2 = wk0 | wk1;
			if (dev->_flip)
				wk2 = ssd1306_rotate_byte(wk2);
			dev->_page[0]._segs[seg] = wk2;
		}
	}
	else if (scroll == PAGE_SCROLL_DOWN)
	{
		uint8_t save[128];
		// Save pages 7
		for (int seg = 0; seg < 128; seg++)
		{
			save[seg] = dev->_page[dev->_pages - 1]._segs[seg];
		}
		// Page7 to Page1
		for (int page = dev->_pages - 1; page > 0; page--)
		{
			for (int seg = 0; seg < 128; seg++)
			{
				dev->_page[page]._segs[seg] = dev->_page[page - 1]._segs[seg];
			}
		}
		// Store  pages 0
		for (int seg = 0; seg < 128; seg++)
		{
			dev->_page[0]._segs[seg] = save[seg];
		}
	}
	else if (scroll == PAGE_SCROLL_UP)
	{
		uint8_t save[128];
		// Save pages 0
		for (int seg = 0; seg < 128; seg++)
		{
			save[seg] = dev->_page[0]._segs[seg];
		}
		// Page0 to Page6
		for (int page = 0; page < dev->_pages - 1; page++)
		{
			for (int seg = 0; seg < 128; seg++)
			{
				dev->_page[page]._segs[seg] = dev->_page[page + 1]._segs[seg];
			}
		}
		// Store  pages 7
		for (int seg = 0; seg < 128; seg++)
		{
			dev->_page[dev->_pages - 1]._segs[seg] = save[seg];
		}
	}

	if (delay >= 0)
	{
		for (int page = 0; page < dev->_pages; page++)
		{
			i2c_display_image(dev, page, 0, dev->_page[page]._segs, 128);

			if (delay)
				vTaskDelay(delay);
		}
	}
}

void _ssd1306_bitmaps(SSD1306_t *dev, int xpos, int ypos, uint8_t *bitmap, int width, int height, bool invert)
{
	if ((width % 8) != 0)
	{
		ESP_LOGE(TAG, "width must be a multiple of 8");
		return;
	}
	int _width = width / 8;
	uint8_t wk0;
	uint8_t wk1;
	uint8_t wk2;
	uint8_t page = (ypos / 8);
	uint8_t _seg = xpos;
	uint8_t dstBits = (ypos % 8);
	ESP_LOGD(TAG, "_width=%d ypos=%d page=%d dstBits=%d", _width, ypos, page, dstBits);
	int offset = 0;
	for (int _height = 0; _height < height; _height++)
	{
		for (int index = 0; index < _width; index++)
		{
			for (int srcBits = 7; srcBits >= 0; srcBits--)
			{
				wk0 = dev->_page[page]._segs[_seg];
				if (dev->_flip)
					wk0 = ssd1306_rotate_byte(wk0);

				wk1 = bitmap[index + offset];
				if (invert)
					wk1 = ~wk1;

				// wk2 = ssd1306_copy_bit(bitmap[index+offset], srcBits, wk0, dstBits);
				wk2 = ssd1306_copy_bit(wk1, srcBits, wk0, dstBits);
				if (dev->_flip)
					wk2 = ssd1306_rotate_byte(wk2);

				ESP_LOGD(TAG, "index=%d offset=%d wk1=0x%x page=%d _seg=%d, wk2=%02x", index, offset, wk1, page, _seg, wk2);
				dev->_page[page]._segs[_seg] = wk2;
				_seg++;
			}
		}
		vTaskDelay(1);
		offset = offset + _width;
		dstBits++;
		_seg = xpos;
		if (dstBits == 8)
		{
			page++;
			dstBits = 0;
		}
	}

#if 0
	for (int _seg=ypos;_seg<ypos+width;_seg++) {
		ssd1306_dump_page(dev, page-1, _seg);
	}
	for (int _seg=ypos;_seg<ypos+width;_seg++) {
		ssd1306_dump_page(dev, page, _seg);
	}
#endif
	// ssd1306_show_buffer(dev);
}

void ssd1306_bitmaps(SSD1306_t *dev, int xpos, int ypos, uint8_t *bitmap, int width, int height, bool invert)
{
	_ssd1306_bitmaps(dev, xpos, ypos, bitmap, width, height, invert);
	ssd1306_show_buffer(dev);
}

// Set pixel to internal buffer. Not show it.
void _ssd1306_pixel(SSD1306_t *dev, int xpos, int ypos, bool invert)
{
	uint8_t _page = (ypos / 8);
	uint8_t _bits = (ypos % 8);
	uint8_t _seg = xpos;
	uint8_t wk0 = dev->_page[_page]._segs[_seg];
	uint8_t wk1 = 1 << _bits;
	ESP_LOGD(TAG, "ypos=%d _page=%d _bits=%d wk0=0x%02x wk1=0x%02x", ypos, _page, _bits, wk0, wk1);
	if (invert)
	{
		wk0 = wk0 & ~wk1;
	}
	else
	{
		wk0 = wk0 | wk1;
	}
	if (dev->_flip)
		wk0 = ssd1306_rotate_byte(wk0);
	ESP_LOGD(TAG, "wk0=0x%02x wk1=0x%02x", wk0, wk1);
	dev->_page[_page]._segs[_seg] = wk0;
}

// Set line to internal buffer. Not show it.
void _ssd1306_line(SSD1306_t *dev, int x1, int y1, int x2, int y2, bool invert)
{
	int i;
	int dx, dy;
	int sx, sy;
	int E;

	/* distance between two points */
	dx = (x2 > x1) ? x2 - x1 : x1 - x2;
	dy = (y2 > y1) ? y2 - y1 : y1 - y2;

	/* direction of two point */
	sx = (x2 > x1) ? 1 : -1;
	sy = (y2 > y1) ? 1 : -1;

	/* inclination < 1 */
	if (dx > dy)
	{
		E = -dx;
		for (i = 0; i <= dx; i++)
		{
			_ssd1306_pixel(dev, x1, y1, invert);
			x1 += sx;
			E += 2 * dy;
			if (E >= 0)
			{
				y1 += sy;
				E -= 2 * dx;
			}
		}

		/* inclination >= 1 */
	}
	else
	{
		E = -dy;
		for (i = 0; i <= dy; i++)
		{
			_ssd1306_pixel(dev, x1, y1, invert);
			y1 += sy;
			E += 2 * dx;
			if (E >= 0)
			{
				x1 += sx;
				E -= 2 * dy;
			}
		}
	}
}

// Draw circle
void _ssd1306_circle(SSD1306_t *dev, int x0, int y0, int r, bool invert)
{
	int x;
	int y;
	int err;
	int old_err;

	x = 0;
	y = -r;
	err = 2 - 2 * r;
	do
	{
		_ssd1306_pixel(dev, x0 - x, y0 + y, invert);
		_ssd1306_pixel(dev, x0 - y, y0 - x, invert);
		_ssd1306_pixel(dev, x0 + x, y0 - y, invert);
		_ssd1306_pixel(dev, x0 + y, y0 + x, invert);
		if ((old_err = err) <= x)
			err += ++x * 2 + 1;
		if (old_err > y || err > x)
			err += ++y * 2 + 1;
	} while (y < 0);
}

// Draw cursor
void _ssd1306_cursor(SSD1306_t *dev, int x0, int y0, int r, bool invert)
{
	_ssd1306_line(dev, x0 - r, y0, x0 + r, y0, invert);
	_ssd1306_line(dev, x0, y0 - r, x0, y0 + r, invert);
}

void ssd1306_invert(uint8_t *buf, size_t blen)
{
	uint8_t wk;
	for (int i = 0; i < blen; i++)
	{
		wk = buf[i];
		buf[i] = ~wk;
	}
}

// Flip upside down
void ssd1306_flip(uint8_t *buf, size_t blen)
{
	for (int i = 0; i < blen; i++)
	{
		buf[i] = ssd1306_rotate_byte(buf[i]);
	}
}

uint8_t ssd1306_copy_bit(uint8_t src, int srcBits, uint8_t dst, int dstBits)
{
	ESP_LOGD(TAG, "src=%02x srcBits=%d dst=%02x dstBits=%d", src, srcBits, dst, dstBits);
	uint8_t smask = 0x01 << srcBits;
	uint8_t dmask = 0x01 << dstBits;
	uint8_t _src = src & smask;
#if 0
	if (_src != 0) _src = 1;
	uint8_t _wk = _src << dstBits;
	uint8_t _dst = dst | _wk;
#endif
	uint8_t _dst;
	if (_src != 0)
	{
		_dst = dst | dmask; // set bit
	}
	else
	{
		_dst = dst & ~(dmask); // clear bit
	}
	return _dst;
}

// Rotate 8-bit data
// 0x12-->0x48
uint8_t ssd1306_rotate_byte(uint8_t ch1)
{
	uint8_t ch2 = 0;
	for (int j = 0; j < 8; j++)
	{
		ch2 = (ch2 << 1) + (ch1 & 0x01);
		ch1 = ch1 >> 1;
	}
	return ch2;
}

void ssd1306_fadeout(SSD1306_t *dev)
{
	void (*func)(SSD1306_t *dev, int page, int seg, uint8_t *images, int width);
	func = i2c_display_image;

	uint8_t image[1];
	for (int page = 0; page < dev->_pages; page++)
	{
		image[0] = 0xFF;
		for (int line = 0; line < 8; line++)
		{
			if (dev->_flip)
			{
				image[0] = image[0] >> 1;
			}
			else
			{
				image[0] = image[0] << 1;
			}
			for (int seg = 0; seg < 128; seg++)
			{
				(*func)(dev, page, seg, image, 1);
				dev->_page[page]._segs[seg] = image[0];
			}
		}
	}
}

// Rotate character image
// Only valid for 8 dots x 8 dots
void ssd1306_rotate_image(uint8_t *image, bool flip)
{
	uint8_t _image[8];
	uint8_t _smask = 0x01;
	for (int i = 0; i < 8; i++)
	{
		uint8_t _dmask = 0x80;
		_image[i] = 0;
		for (int j = 0; j < 8; j++)
		{
			uint8_t _wk = image[j] & _smask;
			ESP_LOGD(TAG, "image[%d]=0x%x _smask=0x%x _wk=0x%x", j, image[j], _smask, _wk);
			if (_wk != 0)
			{
				_image[i] = _image[i] + _dmask;
			}
			_dmask = _dmask >> 1;
		}
		_smask = _smask << 1;
	}

	for (int i = 0; i < 8; i++)
	{
		image[i] = _image[i];
	}
	if (flip)
		ssd1306_flip(image, 8);
#if 0
	for (int i=0;i<8;i++) {
		ESP_LOGI(TAG, "image[%d]=0x%x", i, image[i]);
	}
#endif
}

void ssd1306_display_rotate_text(SSD1306_t *dev, int seg, char *text, int text_len, bool invert)
{
	int _text_len = text_len;
	if (_text_len > 8)
		_text_len = 8;
	uint8_t image[8];
	int _page = dev->_pages - 1;
	for (uint8_t i = 0; i < _text_len; i++)
	{
		memcpy(image, font8x8_basic_tr[(uint8_t)text[i]], 8);
		ssd1306_rotate_image(image, dev->_flip);
		ESP_LOGD(TAG, "_page=%d seg=%d", _page, seg);
		if (invert)
			ssd1306_invert(image, 8);
		ssd1306_display_image(dev, _page, seg, image, 8);
		_page--;
		if (_page < 0)
			return;
	}
}

void ssd1306_dump(SSD1306_t dev)
{
	printf("_address=%x\n", dev._address);
	printf("_width=%x\n", dev._width);
	printf("_height=%x\n", dev._height);
	printf("_pages=%x\n", dev._pages);
}

void ssd1306_dump_page(SSD1306_t *dev, int page, int seg)
{
	ESP_LOGI(TAG, "dev->_page[%d]._segs[%d]=%02x", page, seg, dev->_page[page]._segs[seg]);
}


void i2c_display_image(SSD1306_t *dev, int page, int seg, uint8_t *images, int width)
{
	if (page >= dev->_pages)
		return;
	if (seg >= dev->_width)
		return;

	int _seg = seg + CONFIG_OFFSETX;
	uint8_t columLow = _seg & 0x0F;
	uint8_t columHigh = (_seg >> 4) & 0x0F;

	int _page = page;
	if (dev->_flip)
	{
		_page = (dev->_pages - page) - 1;
	}

	uint8_t *out_buf;
	out_buf = malloc(width < 4 ? 4 : width + 1);
	if (out_buf == NULL)
	{
		ESP_LOGE(TAG, "malloc fail");
		return;
	}
	int out_index = 0;
	out_buf[out_index++] = OLED_CONTROL_BYTE_CMD_STREAM;
	// Set Lower Column Start Address for Page Addressing Mode
	out_buf[out_index++] = (0x00 + columLow);
	// Set Higher Column Start Address for Page Addressing Mode
	out_buf[out_index++] = (0x10 + columHigh);
	// Set Page Start Address for Page Addressing Mode
	out_buf[out_index++] = 0xB0 | _page;

	esp_err_t res;
	res = i2c_master_transmit(dev->_i2c_dev_handle, out_buf, out_index, I2C_TICKS_TO_WAIT);
	if (res != ESP_OK)
		ESP_LOGE(TAG, "Could not write to device [0x%02x at %d]: %d (%s)", dev->_address, dev->_i2c_num, res, esp_err_to_name(res));

	out_buf[0] = OLED_CONTROL_BYTE_DATA_STREAM;
	memcpy(&out_buf[1], images, width);

	res = i2c_master_transmit(dev->_i2c_dev_handle, out_buf, width + 1, I2C_TICKS_TO_WAIT);
	if (res != ESP_OK)
		ESP_LOGE(TAG, "Could not write to device [0x%02x at %d]: %d (%s)", dev->_address, dev->_i2c_num, res, esp_err_to_name(res));
	free(out_buf);
}
