﻿#ifndef __GMCONTROLTEXTEDIT_H__
#define __GMCONTROLTEXTEDIT_H__
#include <gmcommon.h>
#include <gmcontrols.h>
BEGIN_NS

class GMTypoTextBuffer;
class GMMultiLineTypoTextBuffer;

GM_PRIVATE_OBJECT(GMControlTextEdit)
{
	GMVec4 textColor = GMVec4(0, 0, 0, 1);
	GMVec4 selectionBackColor = GMVec4(.15f, .31f, .47f, 1);
	GMVec4 caretColor = GMVec4(0, 0, 0, 1);
	GMint cp = 0;
	GMint firstVisibleCP = 0;
	GMint selectionStartCP = 0;
	GMTypoTextBuffer* buffer = nullptr;
	GMRect rcText;
	GMStyle textStyle;
	GMControlBorder* borderControl = nullptr;
	GMint borderWidth = 5;
	GMfloat lastBlink = 0;
	GMfloat deltaBlink = .5f;
	bool caretOn = true;
	bool showCaret = true;
	bool insertMode = true;
	GMString renderText;
	bool mouseDragging = false;
	GMint padding[2] = { 5, 10 };
};

class GMControlTextEdit : public GMControl
{
	GM_DECLARE_PRIVATE_AND_BASE(GMControlTextEdit, GMControl)

public:
	GM_DECLARE_SIGNAL(textChanged);

public:
	GMControlTextEdit(GMWidget* widget);
	~GMControlTextEdit();

public:
	virtual void render(GMfloat elapsed) override;
	virtual void setSize(GMint width, GMint height) override;
	virtual void setPosition(GMint x, GMint y) override;
	virtual bool onKeyDown(GMSystemKeyEvent* event) override;
	virtual bool onMouseDown(GMSystemMouseEvent* event) override;
	virtual bool onMouseDblClick(GMSystemMouseEvent* event) override;
	virtual bool onMouseUp(GMSystemMouseEvent* event) override;
	virtual bool onMouseMove(GMSystemMouseEvent* event) override;
	virtual bool onChar(GMSystemCharEvent* event) override;
	virtual void onMouseEnter() override;
	virtual void onMouseLeave() override;
	virtual bool canHaveFocus() override;
	virtual void setText(const GMString& text);
	virtual void setPadding(GMint x, GMint y);

protected:
	virtual void renderCaret(GMint firstX, GMint caretX);
	virtual void pasteFromClipboard();
	virtual GMint getCaretTop();
	virtual void handleMouseCaret(const GMPoint& pt, bool selectStart);

protected:
	void createBufferTypoEngineIfNotExist();
	void blinkCaret(GMint firstX, GMint caretX);
	void placeCaret(GMint cP);
	void moveCaret(bool next, bool newItem, bool select);
	void deleteSelectionText();
	void selectAll();
	void resetCaretBlink();
	void copyToClipboard();
	void handleMouseSelect(GMSystemMouseEvent* event, bool selectStart);
	void initStyles(GMWidget* widget);
	GMint getCaretHeight();
	void moveFirstVisibleCp(GMint distance);

public:
	inline GMControlBorder* getBorder() GM_NOEXCEPT
	{
		D(d);
		return d->borderControl;
	}

	inline void setBorderWidth(GMint width) GM_NOEXCEPT
	{
		D(d);
		d->borderWidth = width;
	}

	inline void setShowCaret(bool showCaret) GM_NOEXCEPT
	{
		D(d);
		d->showCaret = showCaret;
	}

	inline void setCaretBlinkSpeed(GMfloat blinkSpeedSecond) GM_NOEXCEPT
	{
		D(d);
		d->deltaBlink = blinkSpeedSecond;
	}

	inline const GMString& getRenderText() GM_NOEXCEPT
	{
		D(d);
		return d->renderText;
	}

protected:
	virtual void updateRect() override;
	virtual void calculateRenderText(GMint firstX);
	virtual void insertCharacter(GMwchar ch);
};

GM_PRIVATE_OBJECT(GMControlTextArea)
{
	GMint caretTopRelative = 0;
	GMMultiLineTypoTextBuffer* buffer = nullptr;
};

class GMControlTextArea : public GMControlTextEdit
{
	GM_DECLARE_PRIVATE_AND_BASE(GMControlTextArea, GMControlTextEdit)

public:
	GMControlTextArea(GMWidget* widget);

public:
	virtual void render(GMfloat elapsed) override;
	virtual void pasteFromClipboard() override;
	virtual void insertCharacter(GMwchar ch) override;
	virtual void setSize(GMint width, GMint height) override;
	virtual GMint getCaretTop() override;
	virtual void handleMouseCaret(const GMPoint& pt, bool selectStart) override;

private:
	inline void setCaretTopRelative(GMint caretTopRelative) GM_NOEXCEPT
	{
		D(d);
		d->caretTopRelative = caretTopRelative;
	}
};

END_NS
#endif