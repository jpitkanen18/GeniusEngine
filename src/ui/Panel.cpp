#include "Panel.hpp"
#include "UI.hpp"

namespace GE::UI
{

	void Panel::draw(UI &ctx)
	{
		if (!this->m_visible)
			return;

		this->m_ctx = &ctx;
		if (ctx.beginPanel(this->m_title))
		{
			layout();
		}
		ctx.endPanel();
		this->m_ctx = nullptr;
	}

	// --- Widget forwarding ---

	void Panel::label(const std::string &text) { this->m_ctx->text(text); }
	void Panel::labelColored(const Color &color, const std::string &text) { this->m_ctx->textColored(color, text); }
	void Panel::separator() { this->m_ctx->separator(); }
	void Panel::spacing() { this->m_ctx->spacing(); }
	void Panel::sameLine(float offset) { this->m_ctx->sameLine(offset); }

	bool Panel::button(const std::string &lbl, const Vec2 &size) { return this->m_ctx->button(lbl, size); }
	bool Panel::checkbox(const std::string &lbl, bool &value) { return this->m_ctx->checkbox(lbl, value); }
	bool Panel::slider(const std::string &lbl, float &value, float min, float max) { return this->m_ctx->sliderFloat(lbl, value, min, max); }
	bool Panel::slider3(const std::string &lbl, Vec3 &value, float min, float max) { return this->m_ctx->sliderFloat3(lbl, value, min, max); }
	bool Panel::colorEdit(const std::string &lbl, Vec3 &color) { return this->m_ctx->colorEdit3(lbl, color); }
	bool Panel::inputFloat(const std::string &lbl, float &value, float step) { return this->m_ctx->inputFloat(lbl, value, step); }

	bool Panel::beginCombo(const std::string &lbl, const std::string &preview) { return this->m_ctx->beginCombo(lbl, preview); }
	bool Panel::selectableItem(const std::string &lbl, bool selected) { return this->m_ctx->selectableItem(lbl, selected); }
	void Panel::endCombo() { this->m_ctx->endCombo(); }

	bool Panel::treeNode(const std::string &lbl) { return this->m_ctx->treeNode(lbl); }
	void Panel::treePop() { this->m_ctx->treePop(); }

	void Panel::progressBar(float fraction, const std::string &overlay) { this->m_ctx->progressBar(fraction, overlay); }

	void Panel::section(const std::string &lbl, std::function<void()> body)
	{
		if (this->m_ctx->treeNode(lbl))
		{
			body();
			this->m_ctx->treePop();
		}
	}

} // namespace GE::UI
