/*=========================================================================

   Program: ParaView
   Module:    pqLineChartPlotOptions.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

/// \file pqLineChartPlotOptions.h
/// \date 9/18/2006

#ifndef _pqLineChartPlotOptions_h
#define _pqLineChartPlotOptions_h


#include "QtChartExport.h"
#include <QObject>

class pqLineChartPlotOptionsInternal;
class pqPointMarker;
class QBrush;
class QPainter;
class QPen;


/// \class pqLineChartPlotOptions
/// \brief
///   The pqLineChartPlotOptions class stores the drawing options for
///   a line chart plot.
class QTCHART_EXPORT pqLineChartPlotOptions : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a line chart plot options object.
  /// \param parent The parent object.
  pqLineChartPlotOptions(QObject *parent=0);
  ~pqLineChartPlotOptions();

  /// \brief
  ///   Sets the pen to use when drawing the plot series.
  /// \param series The index of the series in the plot.
  /// \param pen The pen to use when drawing the series.
  void setPen(int series, const QPen &pen);

  /// \brief
  ///   Sets the brush to use when filling series points.
  /// \param series The index of the series in the plot.
  /// \param brush The brush to use when filling series points.
  void setBrush(int series, const QBrush &brush);

  /// \brief
  ///   Sets the marker to use when drawing series points.
  /// \param series The index of the series in the plot.
  /// \param marker The marker to use when drawing series points.
  void setMarker(int series, pqPointMarker *marker);

  /// \brief
  ///   Sets up the painter for drawing a series.
  ///
  /// The pen and brush for a given series are set on the painter.
  ///
  /// \param painter The painter to set up.
  /// \param series The index of the series in the plot.
  void setupPainter(QPainter &painter, int series) const;

  /// \brief
  ///   Gets the point marker for a series.
  /// \param series The index of the series in the plot.
  /// \return
  ///   A pointer to the point marker for a series.
  pqPointMarker *getMarker(int series) const;

signals:
  /// Emitted when the drawing options for a plot have changed.
  void optionsChanged();

private:
  pqLineChartPlotOptionsInternal *Internal; ///< Stores the options.
};

#endif
