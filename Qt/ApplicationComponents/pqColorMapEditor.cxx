/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
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

========================================================================*/
#include "pqColorMapEditor.h"
#include "ui_pqColorMapEditor.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqProxyWidgetDialog.h"
#include "pqProxyWidget.h"
#include "pqScalarBarVisibilityReaction.h"
#include "pqSearchBox.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"

#include "vtkCommand.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMSettings.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkWeakPointer.h"

#include <QDebug>
#include <QKeyEvent>
#include <QPointer>
#include <QVBoxLayout>

class pqColorMapEditor::pqInternals
{
public:
  Ui::ColorMapEditor Ui;
  QPointer<pqProxyWidget> ProxyWidget;
  QPointer<pqDataRepresentation> ActiveRepresentation;
  QPointer<QAction> ScalarBarVisibilityAction;
  unsigned long ObserverId;

  pqInternals(pqColorMapEditor* self) : ObserverId(0)
    {
    this->Ui.setupUi(self);

    QVBoxLayout* vbox = new QVBoxLayout(this->Ui.PropertiesFrame);
    vbox->setMargin(0);
    vbox->setSpacing(0);
    }

  ~pqInternals()
    {
    }
};

//-----------------------------------------------------------------------------
pqColorMapEditor::pqColorMapEditor(QWidget* parentObject)
  : Superclass(parentObject),
  Internals(new pqColorMapEditor::pqInternals(this))
{
  QObject::connect(this->Internals->Ui.SearchBox,
                   SIGNAL(advancedSearchActivated(bool)),
                   this, SLOT(updatePanel()));
  QObject::connect(this->Internals->Ui.SearchBox, SIGNAL(textChanged(QString)),
                   this, SLOT(updatePanel()));
  QObject::connect(this->Internals->Ui.EditScalarBar, SIGNAL(clicked()),
                   this, SLOT(editScalarBar()));
  QObject::connect(this->Internals->Ui.SaveAsDefault, SIGNAL(clicked()),
                   this, SLOT(saveAsDefault()));
  QObject::connect(this->Internals->Ui.AutoUpdate, SIGNAL(clicked(bool)),
                   this, SLOT(setAutoUpdate(bool)));
  QObject::connect(this->Internals->Ui.Update, SIGNAL(clicked()),
                   this, SLOT(renderViews()));

  // Let pqScalarBarVisibilityReaction do the heavy lifting for managing the
  // show-scalar bar button.
  QAction* showSBAction = new QAction(this);
  this->Internals->ScalarBarVisibilityAction = showSBAction;
  this->Internals->Ui.ShowScalarBar->connect(
    showSBAction, SIGNAL(toggled(bool)), SLOT(setChecked(bool)));
  showSBAction->connect(
    this->Internals->Ui.ShowScalarBar, SIGNAL(clicked(bool)), SLOT(trigger()));
  this->connect(showSBAction, SIGNAL(changed()), SLOT(updateScalarBarButtons()));
  new pqScalarBarVisibilityReaction(showSBAction);

  pqActiveObjects *activeObjects = &pqActiveObjects::instance();
  this->connect(activeObjects, SIGNAL(representationChanged(pqDataRepresentation*)),
    this, SLOT(updateActive()));

  pqSettings *settings = pqApplicationCore::instance()->settings();
  if (settings)
    {
    this->Internals->Ui.AutoUpdate->setChecked(
      settings->value("autoUpdateColorMapEditor", false).toBool());
    }
  this->updateActive();
}

//-----------------------------------------------------------------------------
pqColorMapEditor::~pqColorMapEditor()
{
  pqSettings *settings = pqApplicationCore::instance()->settings();
  if (settings)
    {
    // save the state of advanced button in the user config.
    settings->setValue("autoUpdateColorMapEditor",
      this->Internals->Ui.AutoUpdate->isChecked());
    }

  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updatePanel()
{
  if (this->Internals->ProxyWidget)
    {
    this->Internals->ProxyWidget->filterWidgets(
      this->Internals->Ui.SearchBox->isAdvancedSearchActive(),
      this->Internals->Ui.SearchBox->text());
    }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateActive()
{
  pqDataRepresentation* repr =
    pqActiveObjects::instance().activeRepresentation();

  this->setDataRepresentation(repr);

  // Set the current LUT proxy to edit.
  if (repr && vtkSMPVRepresentationProxy::GetUsingScalarColoring(repr->getProxy()))
    {
    this->setColorTransferFunction(
      vtkSMPropertyHelper(repr->getProxy(), "LookupTable", true).GetAsProxy());
    }
  else
    {
    this->setColorTransferFunction(NULL);
    }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setDataRepresentation(pqDataRepresentation* repr)
{
  // this method sets up hooks to ensure that when the repr's properties are
  // modified, the editor shows the correct LUT.
  if (this->Internals->ActiveRepresentation == repr)
    {
    return;
    }

  if (this->Internals->ActiveRepresentation)
    {
    // disconnect signals.
    if (this->Internals->ObserverId)
      {
      this->Internals->ActiveRepresentation->getProxy()->RemoveObserver(
        this->Internals->ObserverId);
      }
    }

  this->Internals->ObserverId = 0;
  this->Internals->ActiveRepresentation = repr;
  if (repr && repr->getProxy())
    {
    this->Internals->ObserverId = repr->getProxy()->AddObserver(
      vtkCommand::PropertyModifiedEvent, this, &pqColorMapEditor::updateActive);
    }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setColorTransferFunction(vtkSMProxy* ctf)
{
  Ui::ColorMapEditor& ui = this->Internals->Ui;

  if (this->Internals->ProxyWidget == NULL && ctf == NULL)
    {
    return;
    }
  if (this->Internals->ProxyWidget && ctf &&
    this->Internals->ProxyWidget->proxy() == ctf)
    {
    return;
    }

  if ( (ctf==NULL && this->Internals->ProxyWidget) ||
       (this->Internals->ProxyWidget && ctf && this->Internals->ProxyWidget->proxy() != ctf))
    {
    ui.PropertiesFrame->layout()->removeWidget(this->Internals->ProxyWidget);
    delete this->Internals->ProxyWidget;
    }

  ui.SaveAsDefault->setEnabled(ctf != NULL);
  if (!ctf)
    {
    return;
    }

  pqProxyWidget* widget = new pqProxyWidget(ctf, this);
  widget->setObjectName("Properties");
  widget->setApplyChangesImmediately(true);
  widget->filterWidgets();

  ui.PropertiesFrame->layout()->addWidget(widget);

  this->Internals->ProxyWidget = widget;
  this->updatePanel();

  QObject::connect(widget, SIGNAL(changeFinished()), this, SLOT(updateIfNeeded()));
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateScalarBarButtons()
{
  Ui::ColorMapEditor& ui = this->Internals->Ui;
  bool can_show_sb = this->Internals->ScalarBarVisibilityAction->isEnabled();
  ui.ShowScalarBar->setEnabled(can_show_sb);
  ui.EditScalarBar->setEnabled(can_show_sb &&
    this->Internals->ScalarBarVisibilityAction->isChecked());
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::editScalarBar()
{
  Q_ASSERT(this->Internals->ProxyWidget && this->Internals->ActiveRepresentation);

  vtkSMProxy* lutProxy = this->Internals->ProxyWidget->proxy();
  vtkSMProxy* viewProxy = this->Internals->ActiveRepresentation->getView()->getProxy();
  vtkSMProxy* sbProxy = vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, viewProxy);
  if (sbProxy)
    {
    pqProxyWidgetDialog dialog(sbProxy);
    QObject::connect(&dialog, SIGNAL(accepted()),
      this, SLOT(renderViews()));
    dialog.setWindowTitle("Edit Color Legend Parameters");
    dialog.setObjectName("ColorLegendEditor");
    dialog.exec();
    }
  else
    {
    qCritical("Failed to locate scalar bar proxy. Ignoring.");
    }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::renderViews()
{
  if (this->Internals->ActiveRepresentation)
    {
    this->Internals->ActiveRepresentation->renderViewEventually();
    }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::saveAsDefault()
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  vtkSMProxy* proxy = this->Internals->ActiveRepresentation->getProxy();

  vtkSMProxy* lutProxy =
    pqSMAdaptor::getProxyProperty(proxy->GetProperty("LookupTable"));
  if (lutProxy)
    {
    settings->SetProxySettings(lutProxy);
    }
  else
    {
    qCritical() << "No LookupTable property found.";
    }

  vtkSMProxy* scalarOpacityFunctionProxy =
    pqSMAdaptor::getProxyProperty(proxy->GetProperty("ScalarOpacityFunction"));
  if (scalarOpacityFunctionProxy)
    {
    settings->SetProxySettings(scalarOpacityFunctionProxy);
    }
  else
    {
    qCritical("No ScalarOpacityFunction property found");
    }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setAutoUpdate(bool val)
{
  this->Internals->Ui.AutoUpdate->setChecked(val);
  this->updateIfNeeded();
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateIfNeeded()
{
  if (this->Internals->Ui.AutoUpdate->isChecked())
    {
    this->renderViews();
    }
}
