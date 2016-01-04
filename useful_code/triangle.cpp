/*
 * Copyright (C) 2009 Josh A. Beam
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Own header
#include "triangle.h"

// Deadfrog lib headers
#include "lib/bitmap.h"

// Standard headers
#include <memory.h>


typedef unsigned uint32_t;


class Color
{
public:
    float R, G, B, A;

    Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);
    Color(RGBAColour const &o) 
    { 
        R = o.r / 255.0f;
        G = o.g / 255.0f;
        B = o.b / 255.0f;
        A = o.a / 255.0f;
    }

    uint32_t ToUInt32() const;

    Color operator + (const Color &c) const;
    Color operator - (const Color &c) const;
    Color operator * (float f) const;
};


Color::Color(float r, float g, float b, float a)
{
    R = r;
    G = g;
    B = b;
    A = a;
}

uint32_t Color::ToUInt32() const
{
    uint32_t r = (uint32_t)(R * 255.0f);
    uint32_t g = (uint32_t)(G * 255.0f);
    uint32_t b = (uint32_t)(B * 255.0f);
    uint32_t a = (uint32_t)(A * 255.0f);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

Color Color::operator + (const Color &c) const
{
    return Color(R + c.R, G + c.G, B + c.B, A + c.A);
}

Color Color::operator - (const Color &c) const
{
    return Color(R - c.R, G - c.G, B - c.B, A - c.A);
}

Color Color::operator * (float f) const
{
    return Color(R * f, G * f, B * f, A * f);
}


class Edge
{
public:
    Color Color1, Color2;
    int X1, Y1, X2, Y2;

    Edge(const Color &color1, int x1, int y1, const Color &color2, int x2, int y2);
};


class Span
{
public:
    Color Color1, Color2;
    int X1, X2;

    Span(const Color &color1, int x1, const Color &color2, int x2);
};


Edge::Edge(const Color &color1, int x1, int y1,
           const Color &color2, int x2, int y2)
{
	if(y1 < y2) {
		Color1 = color1;
		X1 = x1;
		Y1 = y1;
		Color2 = color2;
		X2 = x2;
		Y2 = y2;
	} else {
		Color1 = color2;
		X1 = x2;
		Y1 = y2;
		Color2 = color1;
		X2 = x1;
		Y2 = y1;
	}
}

Span::Span(const Color &color1, int x1, const Color &color2, int x2)
{
	if(x1 < x2) {
		Color1 = color1;
		X1 = x1;
		Color2 = color2;
		X2 = x2;
	} else {
		Color1 = color2;
		X1 = x2;
		Color2 = color1;
		X2 = x1;
	}
}


static void DrawSpan(BitmapRGBA *bmp, const Span &span, int y)
{
	int xdiff = span.X2 - span.X1;
	if(xdiff == 0)
		return;

	Color colordiff = span.Color2 - span.Color1;

	float factor = 0.0f;
	float factorStep = 1.0f / (float)xdiff;

	// draw each pixel in the span
	for(int x = span.X1; x < span.X2; x++) {
		Color c1 = span.Color1 + (colordiff * factor);
        RGBAColour c2 = Colour(c1.R * 255.0, c1.G * 255.0, c1.B * 255.0, c1.A * 255.0);
        PutPix(bmp, x, y, c2);
		factor += factorStep;
	}
}


static void DrawSpansBetweenEdges(BitmapRGBA *bmp, const Edge &e1, const Edge &e2)
{
	// calculate difference between the y coordinates
	// of the first edge and return if 0
	float e1ydiff = (float)(e1.Y2 - e1.Y1);
	if(e1ydiff == 0.0f)
		return;

	// calculate difference between the y coordinates
	// of the second edge and return if 0
	float e2ydiff = (float)(e2.Y2 - e2.Y1);
	if(e2ydiff == 0.0f)
		return;

	// calculate differences between the x coordinates
	// and colors of the points of the edges
	float e1xdiff = (float)(e1.X2 - e1.X1);
	float e2xdiff = (float)(e2.X2 - e2.X1);
	Color e1colordiff = (e1.Color2 - e1.Color1);
	Color e2colordiff = (e2.Color2 - e2.Color1);

	// calculate factors to use for interpolation
	// with the edges and the step values to increase
	// them by after drawing each span
	float factor1 = (float)(e2.Y1 - e1.Y1) / e1ydiff;
	float factorStep1 = 1.0f / e1ydiff;
	float factor2 = 0.0f;
	float factorStep2 = 1.0f / e2ydiff;

	// loop through the lines between the edges and draw spans
	for(int y = e2.Y1; y < e2.Y2; y++) {
		// create and draw span
		Span span(e1.Color1 + (e1colordiff * factor1),
		          e1.X1 + (int)(e1xdiff * factor1),
		          e2.Color1 + (e2colordiff * factor2),
		          e2.X1 + (int)(e2xdiff * factor2));
		DrawSpan(bmp, span, y);

		// increase factors
		factor1 += factorStep1;
		factor2 += factorStep2;
	}
}


void DrawTriangle(BitmapRGBA *bmp,
                  RGBAColour &c1, float x1, float y1,
                  RGBAColour &c2, float x2, float y2,
                  RGBAColour &c3, float x3, float y3)
{
    Color color1 = c1;
    Color color2 = c2;
    Color color3 = c3;

	// create edges for the triangle
	Edge edges[3] = {
		Edge(color1, (int)x1, (int)y1, color2, (int)x2, (int)y2),
		Edge(color2, (int)x2, (int)y2, color3, (int)x3, (int)y3),
		Edge(color3, (int)x3, (int)y3, color1, (int)x1, (int)y1)
	};

	int maxLength = 0;
	int longEdge = 0;

	// find edge with the greatest length in the y axis
	for(int i = 0; i < 3; i++) {
		int length = edges[i].Y2 - edges[i].Y1;
		if(length > maxLength) {
			maxLength = length;
			longEdge = i;
		}
	}

	int shortEdge1 = (longEdge + 1) % 3;
	int shortEdge2 = (longEdge + 2) % 3;

	// draw spans between edges; the long edge can be drawn
	// with the shorter edges to draw the full triangle
	DrawSpansBetweenEdges(bmp, edges[longEdge], edges[shortEdge1]);
	DrawSpansBetweenEdges(bmp, edges[longEdge], edges[shortEdge2]);
}
