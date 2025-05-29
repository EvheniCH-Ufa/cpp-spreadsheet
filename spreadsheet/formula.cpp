#include "formula.h"
#include "FormulaAST.h"
#include "cell.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>


using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(expression))
    {
    }
    catch (const std::exception& exc)
    {
        // обрабатываем исключение
        throw FormulaException("Error pri generacii formuly");
    }

    // Возвращает вычисленное значение формулы либо ошибку. На данном этапе
    // мы создали только 1 вид ошибки -- деление на 0.
    Value Evaluate(const SheetInterface& sheet) const override
    {
        try
        {
            return ast_.Execute(sheet);
        }
        catch (const FormulaError& e)
        {
            return e; 
        }
        catch (const std::exception& e)
        {
            //return FormulaError("Error pri calculate formuly: " + std::string(e.what()));
            return FormulaError(FormulaError::Category::Value);
        }
        catch (...)
        {
            return FormulaError(FormulaError::Category::Value);
        }
    }

    // Возвращает выражение, которое описывает формулу.
    // Не содержит пробелов и лишних скобок.
    std::string GetExpression() const override
    {
        try
        {
            std::stringstream ss;
            ast_.PrintFormula(ss);
            std::string result;
            ss >> result;
            return result;
        }
        catch (const std::exception&)
        {
            //throw FormulaError("Error pri calculate formuly");
            throw FormulaError(FormulaError::Category::Value);
        }
    }

    std::vector<Position> GetReferencedCells() const override
    {
/* Для этого в класс FormulaAST добавлено приватное поле  std::forward_list<Position> cells_. */
        
//        PositionUSet set_pos(ast_.GetCells().begin(), ast_.GetCells().end());  // delete dubli
//        std::vector<Position> vec_pos(set_pos.begin(), set_pos.end());         // vector 
//        std::sort(vec_pos.begin(), vec_pos.end());                             // sort без  
//        return vec_pos;

        std::set<Position> set_pos(ast_.GetCells().begin(), ast_.GetCells().end());
        return std::vector<Position> (set_pos.begin(), set_pos.end());
    };

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}

FormulaError::FormulaError(Category category)
    : category_(category)
{}

FormulaError::Category FormulaError::GetCategory() const
{
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const
{
    return rhs.category_ == category_;
}

std::string_view FormulaError::ToString() const
{
    std::string_view res{""};

    switch (category_)
    {
    case FormulaError::Category::Ref:
        res = "Ref";
        break;
    case FormulaError::Category::Value:
        res = "Value";
        break;
    case FormulaError::Category::Arithmetic:
        res = "Arithmetic";
        break;
    }
    return res;
}
//my