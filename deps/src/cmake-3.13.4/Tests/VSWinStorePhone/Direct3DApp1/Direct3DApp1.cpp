#include "pch.h"

#include "BasicTimer.h"
#include "Direct3DApp1.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace concurrency;

Direct3DApp1::Direct3DApp1()
  : m_windowClosed(false)
  , m_windowVisible(true)
{
}

void Direct3DApp1::Initialize(CoreApplicationView ^ applicationView)
{
  applicationView->Activated +=
    ref new TypedEventHandler<CoreApplicationView ^, IActivatedEventArgs ^>(
      this, &Direct3DApp1::OnActivated);

  CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs ^>(
    this, &Direct3DApp1::OnSuspending);

  CoreApplication::Resuming +=
    ref new EventHandler<Platform::Object ^>(this, &Direct3DApp1::OnResuming);

  m_renderer = ref new CubeRenderer();
}

void Direct3DApp1::SetWindow(CoreWindow ^ window)
{
  window->SizeChanged +=
    ref new TypedEventHandler<CoreWindow ^, WindowSizeChangedEventArgs ^>(
      this, &Direct3DApp1::OnWindowSizeChanged);

  window->VisibilityChanged +=
    ref new TypedEventHandler<CoreWindow ^, VisibilityChangedEventArgs ^>(
      this, &Direct3DApp1::OnVisibilityChanged);

  window->Closed +=
    ref new TypedEventHandler<CoreWindow ^, CoreWindowEventArgs ^>(
      this, &Direct3DApp1::OnWindowClosed);

#ifndef PHONE
  window->PointerCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);
#endif

  window->PointerPressed +=
    ref new TypedEventHandler<CoreWindow ^, PointerEventArgs ^>(
      this, &Direct3DApp1::OnPointerPressed);

  window->PointerMoved +=
    ref new TypedEventHandler<CoreWindow ^, PointerEventArgs ^>(
      this, &Direct3DApp1::OnPointerMoved);

  m_renderer->Initialize(CoreWindow::GetForCurrentThread());
}

void Direct3DApp1::Load(Platform::String ^ entryPoint)
{
}

void Direct3DApp1::Run()
{
  BasicTimer ^ timer = ref new BasicTimer();

  while (!m_windowClosed) {
    if (m_windowVisible) {
      timer->Update();
      CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(
        CoreProcessEventsOption::ProcessAllIfPresent);
      m_renderer->Update(timer->Total, timer->Delta);
      m_renderer->Render();
      m_renderer
        ->Present(); // This call is synchronized to the display frame rate.
    } else {
      CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(
        CoreProcessEventsOption::ProcessOneAndAllPending);
    }
  }
}

void Direct3DApp1::Uninitialize()
{
}

void Direct3DApp1::OnWindowSizeChanged(CoreWindow ^ sender,
                                       WindowSizeChangedEventArgs ^ args)
{
  m_renderer->UpdateForWindowSizeChange();
}

void Direct3DApp1::OnVisibilityChanged(CoreWindow ^ sender,
                                       VisibilityChangedEventArgs ^ args)
{
  m_windowVisible = args->Visible;
}

void Direct3DApp1::OnWindowClosed(CoreWindow ^ sender,
                                  CoreWindowEventArgs ^ args)
{
  m_windowClosed = true;
}

void Direct3DApp1::OnPointerPressed(CoreWindow ^ sender,
                                    PointerEventArgs ^ args)
{
  // Insert your code here.
}

void Direct3DApp1::OnPointerMoved(CoreWindow ^ sender, PointerEventArgs ^ args)
{
  // Insert your code here.
}

void Direct3DApp1::OnActivated(CoreApplicationView ^ applicationView,
                               IActivatedEventArgs ^ args)
{
  CoreWindow::GetForCurrentThread()->Activate();
}

void Direct3DApp1::OnSuspending(Platform::Object ^ sender,
                                SuspendingEventArgs ^ args)
{
  // Save app state asynchronously after requesting a deferral. Holding a
  // deferral
  // indicates that the application is busy performing suspending operations.
  // Be
  // aware that a deferral may not be held indefinitely. After about five
  // seconds,
  // the app will be forced to exit.
  SuspendingDeferral ^ deferral = args->SuspendingOperation->GetDeferral();
  m_renderer->ReleaseResourcesForSuspending();

  create_task([this, deferral]() {
    // Insert your code here.

    deferral->Complete();
  });
}

void Direct3DApp1::OnResuming(Platform::Object ^ sender,
                              Platform::Object ^ args)
{
  // Restore any data or state that was unloaded on suspend. By default, data
  // and state are persisted when resuming from suspend. Note that this event
  // does not occur if the app was previously terminated.
  m_renderer->CreateWindowSizeDependentResources();
}

IFrameworkView ^ Direct3DApplicationSource::CreateView()
{
  return ref new Direct3DApp1();
}

[Platform::MTAThread] int main(Platform::Array<Platform::String ^> ^)
{
  auto direct3DApplicationSource = ref new Direct3DApplicationSource();
  CoreApplication::Run(direct3DApplicationSource);
  return 0;
}
