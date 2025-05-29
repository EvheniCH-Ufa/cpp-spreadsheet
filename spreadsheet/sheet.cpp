#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("");
    }

    auto new_cell_uptr = std::make_unique<Cell>(*this); // pustaya
    new_cell_uptr.get()->Set(pos, text);                // sozdali, vse ok

    { // esli cicle, to v topku
        PositionUSet cheked_cells, prev_cells;// { pos }; //ubrat' pos
        if (new_cell_uptr.get()->CheckCycles(cheked_cells, prev_cells))
        {
            throw CircularDependencyException("");
        }
    }

    // esli byla yacheika, to chistim ssylki v ee formule ot nee
    if (table_.count(pos) != 0)   
    {
        for (const auto& item_pos : table_.at(pos).get()->GetReferencedCells()) // berem u staroi vse ssylki
        {
            ((Cell*)(GetCell(item_pos)))->DelDependentCell(pos);                //deletim u vseh vliyayushchih cell tekush pos
        }
    }

    // vo vse ssylki v ee formule pishem curr cell
    for (const auto& item_pos : new_cell_uptr.get()->GetReferencedCells())  //poluchaem vse ccylki is formuly - vliyaushchie
    {
        ((Cell*)(GetCell(item_pos)))->AddDependentCell(pos);                //dobavlyaem im na chto oni vliyayut tekushuyu
    }

    SetSHeetSize({ pos.row + 1, pos.col + 1 });
    AddPrintRange({ pos.row + 1, pos.col + 1 });
    // esli ne bylo yacheiki, to prosto pishem i uhodim
    if (table_.count(pos) == 0)   
    {
        table_[pos] = std::move(new_cell_uptr);
        return;
    }  //ok

    // sbrasyvaem kash_value
    PositionUSet reseted_cell;
    table_.at(pos).get()->ResetValues(reseted_cell);

    for (const auto& item_pos : table_.at(pos).get()->GetDependentCell()) // berem u staroi vse zavisimye cells
    {
        new_cell_uptr.get()->AddDependentCell(item_pos);
    }

    table_[pos] = std::move(new_cell_uptr);

}

CellInterface* Sheet::GetCellData(Position pos) const
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("");
    }

    if (   pos.col >= table_range_.cols
        || pos.row >= table_range_.rows
        || table_.count(pos) == 0)
    {
        return nullptr;
    }
    return (CellInterface*)(table_.at(pos).get());
}


const CellInterface* Sheet::GetCell(Position pos) const
{
    return GetCellData(pos);
}
CellInterface* Sheet::GetCell(Position pos) 
{
    return GetCellData(pos);
}

//очищает ячейку по индексу. Последующий вызов GetCell() для этой ячейки вернёт nullptr.
//При этом может измениться размер минимальной печатной области.
void Sheet::ClearCell(Position pos)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("");
    }

    if (pos.row >= table_range_.rows || pos.col >= table_range_.cols || table_.count(pos) == 0)
    {
        return;
    }

    if (table_.at(pos).get()->GetDependentCell().empty())
    {
        table_.erase(pos);
    }
    else
    {
        table_.at(pos).get()->Clear();
    }

    // esli cell polnost'u vnutri print_range, to ne menajem
    if (pos.row + 1 < print_range_.rows && pos.col + 1 < print_range_.cols)
    {
        return;
    }
    
    Size new_print_range{ -1, -1 };
    for (int i = 0; i < print_range_.rows; ++i)
    {
        for (int j = 0; j < print_range_.cols; ++j)
        {
            if (table_.count({ i, j }) && table_.at({ i, j }).get()->GetText() != "")
            {
                if (i > new_print_range.rows)
                {
                    new_print_range.rows = i;
                }
                if (j > new_print_range.cols)
                {
                    new_print_range.cols = j;
                }
            }
        }
    }
    print_range_.cols = new_print_range.cols + 1;
    print_range_.rows = new_print_range.rows + 1;
}

Size Sheet::GetPrintableSize() const
{
    return print_range_;
}

void Sheet::PrintCellData(std::ostream& output, PrintDataType print_data_type) const
{
    for (int i = 0; i < print_range_.rows; ++i)
    {
        bool is_first_cell{ true };
        for (int j = 0; j < print_range_.cols; ++j)
        {
            if (!is_first_cell)
            {
                output << "\t"sv;
            }
            is_first_cell = false;

            if (table_.count({ i, j }) == 0)
            {
                continue;
            }

            if (print_data_type == PrintDataType::VALUE)
            {
                const auto var_result = table_.at({ i, j }).get()->GetValue();
                if (var_result.index() == 0)
                {
                    output << std::get<0>(var_result);
                }
                else if (var_result.index() == 1)
                {
                    output << std::get<1>(var_result);
                }
                else
                {
                    output << std::get<2>(var_result);
                }
            }
            else
            {
                output << table_.at({ i, j }).get()->GetText();
            }
        }
        output << "\n"sv;
    }
}

void Sheet::PrintValues(std::ostream& output) const
{
    PrintCellData(output, PrintDataType::VALUE);
}

void Sheet::PrintTexts(std::ostream& output) const
{
    PrintCellData(output, PrintDataType::TEXT);
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

// snarugi zadaem i ne parimsya
void Sheet::SetSHeetSize(Size size) noexcept
{
    if (size.cols >= table_range_.cols)
    {
        table_range_.cols = size.cols;
    }

    if (size.rows >= table_range_.rows)
    {
        table_range_.rows = size.rows;
    }
}

void Sheet::AddPrintRange(Size size)
{
    if (size.cols >= print_range_.cols)
    {
        print_range_.cols = size.cols;
    }

    if (size.rows >= table_range_.rows)
    {
        print_range_.rows = size.rows;
    }

    if (max_col_index_.size() < (size_t)size.rows)
    {
        max_col_index_.resize(size.rows);
    }

    if (max_col_index_[size.rows-1] < size.cols-1) // esli max stolbec v etoi stroke men'she nomera stolbca new cell -> update
    {
        max_col_index_[size.rows - 1] = size.cols - 1;
    }
    
    if (max_rows_indexs_.size() < (size_t)size.cols)
    {
        max_rows_indexs_.resize(size.cols);
    }
    if (max_rows_indexs_[size.cols - 1] < size.rows - 1) // esli max stolbec v etoi stroke men'she nomera stolbca new cell -> update
    {
        max_rows_indexs_[size.cols - 1] = size.rows - 1;
    }
}

void Sheet::SetPrintRange(Size size)
{
    print_range_.cols = size.cols;
    table_range_.rows = size.rows;
}//my