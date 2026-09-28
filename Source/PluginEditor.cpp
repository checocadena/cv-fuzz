#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    std::vector<std::byte> toBytes(const char *data, int size)
    {
        const auto *p = reinterpret_cast<const std::byte *>(data);
        return { p, p + size };
    }

    juce::WebBrowserComponent::Options makeOptions(CVFuzzEditor &,
                                                   juce::WebSliderRelay &fuzz, juce::WebSliderRelay &tone,
                                                   juce::WebSliderRelay &level, juce::WebSliderRelay &bias,
                                                   juce::WebBrowserComponent::ResourceProvider provider)
    {
        return juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
                                        .withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)
                                                                .getChildFile("CVFuzzWebView")))
            .withNativeIntegrationEnabled()
            .withResourceProvider(std::move(provider))
            .withOptionsFrom(fuzz)
            .withOptionsFrom(tone)
            .withOptionsFrom(level)
            .withOptionsFrom(bias)
            // Link buttons in the UI open in the user's normal browser; the DAW keeps running.
            .withNativeFunction("openURL", [](const juce::Array<juce::var> &args,
                                              juce::WebBrowserComponent::NativeFunctionCompletion done)
            {
                if (args.size() > 0)
                    juce::URL(args[0].toString()).launchInDefaultBrowser();
                done({});
            })
            // The résumé button opens the bundled PDF in the system viewer (Preview on macOS).
            .withNativeFunction("openResume", [](const juce::Array<juce::var> &,
                                                 juce::WebBrowserComponent::NativeFunctionCompletion done)
            {
                auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                .getChildFile("Checo_Cadena_Resume.pdf");
                file.replaceWithData(BinaryData::Checo_Cadena_Resume_pdf, BinaryData::Checo_Cadena_Resume_pdfSize);
                file.startAsProcess();
                done({});
            });
    }
}

CVFuzzEditor::CVFuzzEditor(CVFuzzAudioProcessor &p)
    : AudioProcessorEditor(&p),
      fuzzProcessor(p),
      webView(makeOptions(*this, fuzzRelay, toneRelay, levelRelay, biasRelay,
                          [this](const auto &url) { return getResource(url); })),
      fuzzAttachment(*p.apvts.getParameter("fuzz"), fuzzRelay, nullptr),
      toneAttachment(*p.apvts.getParameter("tone"), toneRelay, nullptr),
      levelAttachment(*p.apvts.getParameter("level"), levelRelay, nullptr),
      biasAttachment(*p.apvts.getParameter("bias"), biasRelay, nullptr)
{
    addAndMakeVisible(webView);
    webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    setSize(1000, 620);
}

CVFuzzEditor::~CVFuzzEditor() = default;

void CVFuzzEditor::resized()
{
    webView.setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource> CVFuzzEditor::getResource(const juce::String &url)
{
    // JUCE passes either a bare path ("/juce/index.js") or a full URL; normalise to "juce/index.js".
    auto path = url;
    const auto root = juce::WebBrowserComponent::getResourceProviderRoot();
    if (path.startsWith(root))
        path = path.substring(root.length());
    path = path.upToFirstOccurrenceOf("?", false, false).trimCharactersAtStart("/");

    struct Entry { const char *path; const char *data; int size; const char *mime; };
    const Entry entries[] = {
        { "",                              BinaryData::index_html,              BinaryData::index_htmlSize,              "text/html" },
        { "index.html",                    BinaryData::index_html,              BinaryData::index_htmlSize,              "text/html" },
        { "juce/index.js",                 BinaryData::index_js,                BinaryData::index_jsSize,                "text/javascript" },
        { "juce/check_native_interop.js",  BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize, "text/javascript" },
    };

    for (const auto &e : entries)
        if (path == e.path)
            return juce::WebBrowserComponent::Resource{ toBytes(e.data, e.size), e.mime };

    return std::nullopt;
}

// Called on every display refresh. Sends the most recent output, triggered on a rising zero crossing so the
// waveform holds still, to the scope in the UI.
void CVFuzzEditor::pushScope()
{
    constexpr int points = 220, stride = 4, window = points * stride;
    const int size = CVFuzzAudioProcessor::scopeBufferSize;
    const int writePos = fuzzProcessor.scopeWritePos.load(std::memory_order_relaxed);
    const int start = (writePos - window - 1024 + size * 2) % size;

    int trigger = 0;
    for (int i = 1; i < 1024; ++i)
    {
        const float a = fuzzProcessor.scopeBuffer[static_cast<size_t>((start + i - 1) % size)];
        const float b = fuzzProcessor.scopeBuffer[static_cast<size_t>((start + i) % size)];
        if (a < 0.0f && b >= 0.0f) { trigger = i; break; }
    }

    juce::Array<juce::var> samples;
    samples.ensureStorageAllocated(points);
    for (int i = 0; i < points; ++i)
        samples.add(fuzzProcessor.scopeBuffer[static_cast<size_t>((start + trigger + i * stride) % size)]);

    webView.emitEventIfBrowserIsVisible("scope", juce::var(samples));
}
