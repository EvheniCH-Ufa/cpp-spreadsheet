#include "cell.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <optional>
#include <variant>

#include "sheet.h"


class Cell::Impl
{
public:
    virtual ~Impl() = default;

    // Возвращает видимое значение ячейки.
    // В случае текстовой ячейки это её текст (без экранирующих символов). В
    // случае формулы - числовое значение формулы или сообщение об ошибке.
    virtual Value GetValue() const = 0;

    // Возвращает внутренний текст ячейки, как если бы мы начали её
    // редактирование. В случае текстовой ячейки это её текст (возможно,
    // содержащий экранирующие символы). В случае формулы - её выражение.
    virtual std::string GetText() const = 0;
};

class Cell::EmptyImpl : public Cell::Impl // Возвращает все пустое
{
public:
    Cell::Value GetValue() const override
    {
        return "";
    }

    std::string GetText() const override
    {
        return "";
    }
};

class Cell::TextImpl : public Cell::Impl // Возвращает все пустое
{
public:
    TextImpl(std::string str) : txt_(str) { }

    // Возвращает видимое значение ячейки.
    // В случае текстовой ячейки это её текст (без экранирующих символов). В
    Cell::Value GetValue() const override
    {
        if (!txt_.empty() && txt_[0] == ESCAPE_SIGN)
        {
            return txt_.substr(1, txt_.size() - 1);
        }
        return txt_;
    }

    // Возвращает внутренний текст ячейки, как если бы мы начали её
    // редактирование. В случае текстовой ячейки это её текст (возможно, содержащий экранирующие символы)
    std::string GetText() const override
    {
        return txt_;
    }
private:
    std::string txt_;
};

class Cell::FormulaImpl : public Cell::Impl // Возвращает все пустое
{
public:
    FormulaImpl(Sheet& sheet, std::string str)
        : formula_(ParseFormula(str[0] == FORMULA_SIGN ? str.substr(1) : str))
        , txt_((str[0] == FORMULA_SIGN ? "=" : "") + formula_.get()->GetExpression())
        , sheet_(sheet)
    { }

    // Возвращает видимое значение ячейки.
    // В случае текстовой ячейки это её текст (без экранирующих символов). В
    Cell::Value GetValue() const override
    {
        auto var_result = formula_.get()->Evaluate((SheetInterface&) sheet_);
        if (var_result.index() == 0)
        {
            auto result = std::get<0>(var_result);
            if (std::isnan(result) || std::isinf(result))
            {
                return FormulaError(FormulaError::Category::Arithmetic);
            };
            return result;
        }
        else
        {
            return std::get<1>(var_result);
        }
    }


    std::vector<Position> GetReferencedCells()
    {
        return formula_.get()->GetReferencedCells();
    }

    // Возвращает внутренний текст ячейки, как если бы мы начали её
    // редактирование. В случае текстовой ячейки это её текст (возможно, содержащий экранирующие символы)
    std::string GetText() const override
    {
        return txt_;
    }
private:
    std::unique_ptr<FormulaInterface> formula_;  // тут мы храним формулу
    std::string txt_;
    Sheet& sheet_;

};


// Реализуйте следующие методы
Cell::Cell(Sheet& sheet)
    : impl_ (std::make_unique<EmptyImpl>())
    , sheet_(sheet)
{ }

Cell::~Cell() = default;

bool isFloat(const std::string& s) {
    if (s.empty()) return false;

    size_t i = 0;
    bool hasDot = false;
    bool hasDigit = false;

    // Знак
    if (s[i] == '-' || s[i] == '+') i++;

    // Цифры до и после точки
    for (; i < s.size(); ++i) {
        if (s[i] == '.') {
            if (hasDot) return false; // Две точки
            hasDot = true;
        }
        else if (isdigit(static_cast<unsigned char>(s[i]))) {
            hasDigit = true;
        }
        else {
            return false;
        }
    }

    return hasDigit; // Хотя бы одна цифра
}

void Cell::Set(Position pos, std::string text)
{
    pos_ = pos;

    if (text.empty())
    {
        impl_ = std::make_unique<EmptyImpl>();
        return;
    }

    if ((text[0] == FORMULA_SIGN && text.size() > 1 ) || isFloat(text))
    {
        auto tmp = std::move(impl_);
        impl_ = std::make_unique<FormulaImpl>(sheet_, text);

        auto res = impl_.get()->GetValue();
        if (res.index() == 1)
        {
            value_cache_ = std::get<1>(res);
        }
        else
        {
            value_cache_ = std::get<2>(res);
        }
        
        auto ref_cells = ((FormulaImpl*)(impl_.get()))->GetReferencedCells();

        influencing_cells_ = PositionUSet(ref_cells.begin(), ref_cells.end());
    }
    else
    {
        impl_ = std::make_unique<TextImpl>(text);
    }
}

void Cell::AddDependentCell(Position pos)
{
    dependent_cells_.insert(std::move(pos));
}

void Cell::DelDependentCell(Position pos)
{
    dependent_cells_.erase(pos);
}

void Cell::DelInfluencingCell(Position pos)
{
    influencing_cells_.erase(pos);
}

void Cell::Clear()
{
    for (auto& item : dependent_cells_)
    {
        ((Cell*)(sheet_.GetCell(item)))->DelDependentCell(pos_);
    }
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const
{
    if (value_cache_) 
    {
        switch (value_cache_.value().index())
        {
        case 0: return std::get<0>(value_cache_.value()); break; // double
        case 1: return std::get<1>(value_cache_.value()); break; // FormulaError
        default: break;
        }
    }
    return impl_.get()->GetValue();
}

std::string Cell::GetText() const
{
    return impl_.get()->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const
{
    return std::vector<Position>(influencing_cells_.begin(), influencing_cells_.end());
}

PositionUSet Cell::GetDependentCell() const
{
    return dependent_cells_;
}

bool Cell::CheckCycles(PositionUSet& is_checked, const PositionUSet& prev_cells) const
{
    if (prev_cells.count(pos_))
    {
        return true;
    }
    if (is_checked.count(pos_))
    {
        return false;
    }

    // Mozhno bylo by peredat' po znacheniju, no togda jeto popalo by v stek i moglo by peregruzit' ego, a tak - yavno
    PositionUSet curr_prev_cells(prev_cells);
    curr_prev_cells.insert(pos_);
    for (const auto& infl_cell_pos : GetReferencedCells())
    {
        if (((Cell*)(sheet_.GetCell(infl_cell_pos)))->CheckCycles(is_checked, curr_prev_cells))
        {
            return true;
        }
        is_checked.insert(infl_cell_pos);
    }
    return false;
}

void Cell::ResetValues(PositionUSet& reseted_list)
{
    if (reseted_list.count(pos_))
    {
        return;
    }

    ResetValue();
    reseted_list.insert(pos_);
    for (const auto& depend_cell_pos : GetDependentCells())
    {
        ((Cell*)(sheet_.GetCell(depend_cell_pos)))->ResetValues(reseted_list);
    }
}

void Cell::ResetValue()
{
    value_cache_.reset();
}

std::vector<Position> Cell::GetDependentCells() const
{
    return std::vector<Position>(dependent_cells_.begin(), dependent_cells_.end());
}
//my