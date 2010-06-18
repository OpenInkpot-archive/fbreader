/*
 * Copyright (C) 2004-2010 Geometer Plus <contact@geometerplus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <algorithm>

#include "ZLTextArea.h"
#include "ZLTextLineInfo.h"
#include "ZLTextAreaStyle.h"
#include "ZLTextSelectionModel.h"

#include "../../../../fbreader/src/fbreader/FBReader.h"

extern bool link_not_terminated;

int ZLTextArea::rectangleBound(Style &style, const ZLTextParagraphCursor &paragraph, const ZLTextElementRectangle &rectangle, int toCharIndex, bool mainDir) {
	style.setTextStyle(rectangle.Style, rectangle.BidiLevel);
	const ZLTextWord &word = (const ZLTextWord&)paragraph[rectangle.ElementIndex];
	int length = toCharIndex - rectangle.StartCharIndex;
	bool selectHyphenationSign = false;
	if (length >= rectangle.Length) {
		selectHyphenationSign = rectangle.AddHyphenationSign;
		length = rectangle.Length;
	}
	int rectangleLen = (length > 0) ?
		style.wordWidth(word, rectangle.StartCharIndex, length, selectHyphenationSign) : 0;
	return mainDir ? rectangle.XStart + rectangleLen : rectangle.XEnd - rectangleLen;
}

typedef std::vector<ZLTextSelectionModel::Range> RangeVector;

static bool contains(const ZLTextSelectionModel::Range &range, const ZLTextElementRectangle &rectangle) {
	return
		((range.first.ParagraphIndex < rectangle.ParagraphIndex) ||
		 ((range.first.ParagraphIndex == rectangle.ParagraphIndex) &&
			(range.first.ElementIndex <= rectangle.ElementIndex))) &&
		((range.second.ParagraphIndex > rectangle.ParagraphIndex) ||
		 ((range.second.ParagraphIndex == rectangle.ParagraphIndex) &&
			(range.second.ElementIndex >= rectangle.ElementIndex)));
}

static bool strongContains(const ZLTextSelectionModel::Range &range, const ZLTextWordCursor &cursor) {
	const int pn = cursor.paragraphCursor().index();
	const int wn = cursor.elementIndex();
	return
		((range.first.ParagraphIndex < pn) ||
		 ((range.first.ParagraphIndex == pn) &&
			(range.first.ElementIndex < wn))) &&
		((range.second.ParagraphIndex > pn) ||
		 ((range.second.ParagraphIndex == pn) &&
			(range.second.ElementIndex > wn)));
}

static RangeVector::const_iterator findRange(const RangeVector &ranges, const ZLTextElementRectangle &rectangle) {
	// TODO: binary search
	for (RangeVector::const_iterator it = ranges.begin(); it != ranges.end(); ++it) {
		if (contains(*it, rectangle)) {
			return it;
		}
	}
	return ranges.end();
}

static RangeVector::const_iterator strongFindRange(const RangeVector &ranges, const ZLTextWordCursor &cursor) {
	// TODO: binary search
	for (RangeVector::const_iterator it = ranges.begin(); it != ranges.end(); ++it) {
		if (strongContains(*it, cursor)) {
			return it;
		}
	}
	return ranges.end();
}

void ZLTextArea::drawTextLine(Style &style, const ZLTextLineInfo &info, int y, size_t from, size_t to) {
	std::vector<std::vector<int> > selections;

	FBReader &fbreader = FBReader::Instance();
	const ZLTextParagraphCursor &paragraph = info.RealStart.paragraphCursor();

	const ZLTextElementIterator fromIt = myTextElementMap.begin() + from;
	const ZLTextElementIterator toIt = myTextElementMap.begin() + to;

	if (!mySelectionModel.isNull() && !mySelectionModel->isEmpty() && (from != to)) {
		const std::vector<ZLTextSelectionModel::Range> &ranges =
			mySelectionModel->ranges();

		if (!ranges.empty()) {
			RangeVector::const_iterator rt = ranges.end();
			const int top = y + 1;
			int bottom = y + info.Height + info.Descent;
			if (strongFindRange(ranges, info.End) != ranges.end()) {
				bottom += info.VSpaceAfter;
			}
			int left = width() - 1;
			int right = 0;
			const int baseRTL = isRtl() ? 1 : 0;

			for (ZLTextElementIterator it = fromIt; it < toIt; ++it) {
				const ZLTextElementRectangle &rectangle = *it;
				RangeVector::const_iterator rt2 = findRange(ranges, rectangle);
				if (rt2 == rt) {
					if (rt != ranges.end()) {
						const bool mainDir = rectangle.BidiLevel % 2 == baseRTL;
						int r = rectangle.XEnd;
						const ZLTextSelectionModel::BoundElement &bound =
							mainDir ? rt->second : rt->first;
						if (bound.ElementIndex == rectangle.ElementIndex) {
							const ZLTextElement &element = paragraph[rectangle.ElementIndex];
							if (element.kind() == ZLTextElement::WORD_ELEMENT) {
								r = rectangleBound(style, paragraph, rectangle, bound.CharIndex, mainDir);
							}
						}
						right = std::max(right, r);
					}
				} else {
					if (rt != ranges.end()) {
						//drawSelectionRectangle(left, top, right, bottom);
						std::vector<int> s;
						s.push_back(left);
						s.push_back(top);
						s.push_back(right);
						s.push_back(bottom);
						selections.push_back(s);

						left = width() - 1;
						right = 0;
					}
					rt = rt2;
					if (rt != ranges.end()) {
						if ((it == fromIt) &&
								(info.StartBidiLevel % 2 == baseRTL) &&
								strongContains(*rt, info.Start)) {
							left = 0;
						}

						const bool mainDir = rectangle.BidiLevel % 2 == baseRTL;

						int l = rectangle.XStart - 1;
						int r = rectangle.XEnd;

						const ZLTextSelectionModel::BoundElement &rightBound =
							mainDir ? rt->second : rt->first;
						const ZLTextSelectionModel::BoundElement &leftBound =
							mainDir ? rt->first : rt->second;
						if (paragraph[rectangle.ElementIndex].kind() == ZLTextElement::WORD_ELEMENT) {
							if (rightBound.ElementIndex == rectangle.ElementIndex) {
								r = rectangleBound(style, paragraph, rectangle, rightBound.CharIndex, mainDir);
							}
							if (leftBound.ElementIndex == rectangle.ElementIndex) {
								l = rectangleBound(style, paragraph, rectangle, leftBound.CharIndex, mainDir);
							}
						}

						left = std::min(left, l);
						right = std::max(right, r);
					}
				}
			}
			if (rt != ranges.end()) {
				if ((paragraph.index() < (size_t)rt->second.ParagraphIndex) &&
						strongContains(*rt, info.End)) {
					right = width() - 1;
				}
				//drawSelectionRectangle(left, top, right, bottom);
				std::vector<int> s;
				s.push_back(left);
				s.push_back(top);
				s.push_back(right);
				s.push_back(bottom);
				selections.push_back(s);
			}
		}
	}

	y = std::min(y + info.Height, (int)height());
	int x = 0;
	if (!info.NodeInfo.isNull()) {
		drawTreeLines(*info.NodeInfo, x, y, info.Height, info.Descent + info.VSpaceAfter);
	}

	FBReader::HyperlinkCoord cur_link;
	cur_link.id.erase();
	cur_link.x0 = 0;
	cur_link.x1 = 0;
	cur_link.y0 = 0;
	cur_link.y1 = 0;
	cur_link.next = false;

	ZLTextElementIterator it = fromIt;
	for (ZLTextWordCursor pos = info.Start; !pos.equalElementIndex(info.End); pos.nextWord()) {
		const ZLTextElement &element = paragraph[pos.elementIndex()];

//	const int endElementIndex = info.End.elementIndex();
//	for (; (it != toIt) && (it->ElementIndex != endElementIndex); ++it) {
//		const ZLTextElement &element = paragraph[it->ElementIndex];
		ZLTextElement::Kind kind = element.kind();

		if ((pos.elementIndex() == it->ElementIndex) && ((kind == ZLTextElement::WORD_ELEMENT) || (kind == ZLTextElement::IMAGE_ELEMENT))) {
			style.setTextStyle(it->Style, it->BidiLevel);
			const int wx = it->XStart;
			const int wy = it->YEnd - style.elementDescent(element) - style.textStyle()->verticalShift();
			if (kind == ZLTextElement::WORD_ELEMENT) {
				drawWord(style, wx, wy, (const ZLTextWord&)element, it->StartCharIndex, -1, false);
			} else {
				context().drawImage(
					hOffset() + wx, vOffset() + wy,
					*((const ZLTextImageElement&)element).image(),
					width(), height(), ZLPaintContext::SCALE_REDUCE_SIZE
				);
            }
			++it;
		} else if(kind == ZLTextElement::CONTROL_ELEMENT) {
			const ZLTextControlEntry &control = ((const ZLTextControlElement&)element).entry();

			if (control.isHyperlink()) {
				if(control.kind() == 16) {
					// footnote
					std::string id = ((const ZLTextHyperlinkControlEntry&)control).label();
					fbreader.pageFootnotes.push_back(id);

				} else if(control.kind() == 15) {
					//INTERNAL_HYPERLINK
					//printf("hyperlink start\n");

					if(!cur_link.id.empty()) {
						const int y = it->YEnd - style.elementDescent(element) - style.textStyle()->verticalShift();

						cur_link.x1 = it->XEnd;
						cur_link.y1 = y;
						cur_link.y0 = y - info.Height;

						fbreader.pageLinks.push_back(cur_link);

						context().drawLine(cur_link.x0, cur_link.y1, cur_link.x1, cur_link.y1);
					}

					cur_link.id = ((const ZLTextHyperlinkControlEntry&)control).label();
					cur_link.x0 = it->XStart;
				} else {
					//printf("control.kind() %d, id: %s\n", control.kind(), ((const ZLTextHyperlinkControlEntry&)control).label().c_str());
				}

			} else {
				if(control.kind() == 15) {
					if(cur_link.id.empty() || link_not_terminated)
						cur_link.x0 = fromIt->XStart;

					if(cur_link.id.empty() && fbreader.pageLinks.empty()) {
//						for (ZLTextWordCursor pos2(it); pos2.wordIndex() > 0; pos2.previousWord()) {
//						ZLTextElementIterator it2 = it;
//						for (; (it2 != fromIt) && (it->ElementIndex != info.RealStart.elementIndex()); --it) {
						for (ZLTextWordCursor pos2 = pos; pos2.elementIndex() > 0; pos2.previousWord()) {
							const ZLTextElement &element2 = paragraph[it->ElementIndex];
							ZLTextElement::Kind kind2 = element2.kind();
							if(kind2 == ZLTextElement::CONTROL_ELEMENT) {
								const ZLTextControlEntry &control2 = ((const ZLTextControlElement&)element2).entry();
								if (control2.isHyperlink() && (control2.kind() == 15)) {
									cur_link.id = ((const ZLTextHyperlinkControlEntry&)control2).label();
									break;
								}
							}
						}
					}

					ZLTextElementIterator lit = it;
					if(lit != fromIt)
						lit--;

					const int y = lit->YEnd - style.elementDescent(element) - style.textStyle()->verticalShift();

					cur_link.x1 = lit->XEnd;
					cur_link.y1 = y;
					cur_link.y0 = y - info.Height;

					fbreader.pageLinks.push_back(cur_link);

					context().drawLine(cur_link.x0, cur_link.y1, cur_link.x1, cur_link.y1);

					cur_link.id.erase();
					link_not_terminated = false;
				}
			}
		}
	}

	if(!cur_link.id.empty() || link_not_terminated) {
		ZLTextElementIterator lit = it;
		if(lit != fromIt)
			lit--;

		ZLTextWordCursor pos = info.RealStart;

		const ZLTextElement &element = paragraph[info.End.elementIndex()];
		const int y = lit->YEnd - style.elementDescent(element) - style.textStyle()->verticalShift();
		if(cur_link.id.empty()) {
			cur_link.x0 = fromIt->XStart;
		}

		cur_link.x1 = lit->XEnd;
		cur_link.y1 = y;
		cur_link.y0 = y - info.Height;
		cur_link.next = true;

		fbreader.pageLinks.push_back(cur_link);

		context().drawLine(cur_link.x0, cur_link.y1, cur_link.x1, cur_link.y1);

		link_not_terminated = true;
	}

	if (it != toIt) {
		style.setTextStyle(it->Style, it->BidiLevel);
		int start = 0;
		if (info.Start.equalElementIndex(info.End)) {
			start = info.Start.charIndex();
		}
		int len = info.End.charIndex() - start;
		const ZLTextWord &word = (const ZLTextWord&)info.End.element();
		context().setColor(myProperties.color(style.textStyle()->colorStyle()));
		const int x = it->XStart;
		const int y = it->YEnd - style.elementDescent(word) - style.textStyle()->verticalShift();
		drawWord(style, x, y, word, start, len, it->AddHyphenationSign);
	}
	for(unsigned int i = 0; i < selections.size(); i++) {
		std::vector<int> s = selections.at(i);
		drawSelectionRectangle(s.at(0), s.at(1), s.at(2), s.at(3));
	}
}
