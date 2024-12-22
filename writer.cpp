#include "CodeWriter/Writer.h"

#include <map>
#include <string>
#include <algorithm>
#include <iostream>

#include <QFile>
#include <QDebug>

#include "WriterHelper.h"
#include "CodeWriter/Unit.h"
#include "CodeWriter/Arrays.h"
#include "CodeWriter/CwMap.h"
#include "FileWriter.h"

using namespace std;

namespace cw
{

QString Writer::constQualifier(bool value)
{
    return value ? "const " : QString();
}

QString Writer::initVal(const QString & value)
{
    return value.isEmpty() ? QString() : " = " + value;
}

QString Writer::constexprQualifier(bool value)
{
    return value ? "constexpr " : QString();
}

QString Writer::referenceQualifier(bool value)
{
    return value ? "&" : QString();
}

QString Writer::staticQualifier(bool value)
{
    return value ? "static " : QString();
}

QString Writer::virtualQualifier(bool value)
{
    return value ? "virtual " : QString();
}

QString Writer::externQualifier(const Variable value)
{
    if(value.mIsExtern)
        return  QString("extern ");

    return " ";

}

QString Writer::enumName(const QString& name)const
{
    return(name);
}

QString Writer::toHeaderGaurdMacro(const QString& str) const
{
    return toUpper(str);
}

QString Writer::toUpper(const QString& str) const
{
    QString upper = str;

    for(auto& c : upper)
    {
        if(c >= 'a' && c <= 'z')
        {
            c = QChar('A' + (c.toLatin1() - 'a'));
        }
    }

    return upper;
}

bool Writer::writeHeader(const Unit* unit)
{
    bool data_avail = false;

    if (!unit->mMacroName.isEmpty())
        test = toHeaderGaurdMacro(unit->mMacroName);
    else
        test = toHeaderGaurdMacro(unit->mName);

    if(mBanner)
    {
        mH->writeLine(unit->Banner("h"));
    }

    for(const auto& line : unit->mHeaderBanner)
    {
        mH->writeLine("// " + line);
    }

    mH->writeLine();

    mH->writeLine(macro + test + "_H");
    mH->writeLine(macro2 + test + "_H");

    if(!unit->mHeaderIncludes.isEmpty())
    {
        mH->writeLine();

        for(const auto& pair : sorted(unit->mHeaderIncludes))
        {
            mH->writeInclude(pair.second, pair.first);
        }

        data_avail = true;
    }

    if(!unit->mHeaderBeginBlock.isEmpty())
    {
        mH->writeLine();

        for(const auto& statement : unit->mHeaderBeginBlock)
            mH->writeLine(statement);

        data_avail = true;
    }

    if(!unit->mTypes.isEmpty())
    {
        for(const auto& clas : unit->mTypes)
        {
            if(!mStub)
            {
                mH->writeLine();
                writeClassHeader(clas);
            }
            else
            {
                mH->writeLine();
                mH->writeLine(writeClassDeclaration(clas));
                mH->beginBlock();

                for(const auto& func : clas.mMemberFunctions)
                {
                    writeFunctionHeader(func);
                }

                mH->endBlock(true);
            }
        }

        data_avail = true;
    }

    if(!unit->mStructures.isEmpty())
    {
        for(const auto& strc : unit->mStructures)
        {
            mH->writeLine();

            addStructures(strc);
        }

        data_avail = true;
    }

    if(!unit->mEnumerations.isEmpty())
    {
        for(const auto& enums : unit->mEnumerations)
        {
            mH->writeLine();

            addEnum(enums.mName, enums.mValues, enums.mStronglyTyped, enums.mEnumType);
        }

        data_avail = true;
    }

    if(!unit->mVariables.isEmpty())
    {
        mH->writeLine();

        for(const auto& variable : sorted(unit->mVariables))
        {
            mH->writeLine(QString("%1%2%3%4%5%6 %7%8;")
                          .arg(
                              externQualifier(variable),
                              constexprQualifier(variable.mIsConstexpr),
                              staticQualifier(variable.mIsStatic),
                              constQualifier(variable.mIsConst),
                              variable.mType->mName,
                              referenceQualifier(variable.mIsReference),
                              variable.mName,
                              initVal(variable.mInitVal))
                          );
        }

        data_avail = true;
    }

    if(!unit->mArrays.isEmpty())
    {
        for(const auto& arr : unit->mArrays)
        {
            if(!mStub)
            {
                if(arr.mGenerateHeader)
                {
                    mH->writeLine();
                    writeArrayImpl(arr, mH);
                }
            }
            else
            {
                if(arr.mGenerateHeader)
                {
                    mH->writeLine();
                    mH->writeLine(QString("%1%2std::array<%3, %4> %5")
                                  .arg(staticQualifier(arr.mIsStatic),
                                       (arr.mIsConst ? constQualifier(arr.mIsConst) : constexprQualifier(arr.mIsConstExpr)),
                                       arr.mType.first,
                                       arr.mType.second,
                                       arr.mName));
                    mH->beginBlock();
                    mH->endBlock(true);
                }
            }
        }

        data_avail = true;
    }

    if(!unit->mMap.isEmpty())
    {
        for(const auto& map : unit->mMap)
        {
            if(!mStub)
            {
                if(map.mGenerateHeader)
                {
                    mH->writeLine();
                    writeMapImpl(map, mH);
                }
            }
            else
            {
                if(map.mGenerateHeader)
                {
                    mH->writeLine();

                    if(map.mSuppressComment.first)
                        mH->writeLine(QString("// parasoft-begin-suppress  %1 GM FALSE POSITIVE Suppressing manually").arg(map.mSuppressComment.second));

                    mH->writeLine(QString("%1%2std::%6<%3, %4> %5")
                                  .arg(staticQualifier(map.mIsStatic),
                                       constQualifier(map.mIsConst),
                                       map.mType.first,
                                       map.mType.second,
                                       map.mName,(map.mMultiMap ? "multimap" : "map")));
                    mH->beginBlock();
                    mH->endBlock(true);

                    if(map.mSuppressComment.first)
                        mH->writeLine(QString("// parasoft-end-suppress  %1 GM FALSE POSITIVE Suppressing manually").arg(map.mSuppressComment.second));
                }
            }
        }
        data_avail = true;
    }

    if(!unit->mFunctions.isEmpty())
    {
        for(const auto& func : unit->mFunctions)
        {
            mH->writeLine();

            if(func.mGenerateHeader)
            {
                writeLangFunctionImpl(func);
            }
            else
            {
                writeFunctionHeader(func);
            }
        }

        data_avail = true;
    }

    if(!unit->mVectors.isEmpty())
    {
        for(const auto& vector : unit->mVectors)
        {
            if(vector.mGenerateHeader)
            {
                mH->writeLine();
                writeVectorImpl(vector, mH);
            }
        }

        data_avail = true;
    }

    if(!unit->mHeaderEndBlock.isEmpty())
    {
        for(const auto& statement : unit->mHeaderEndBlock)
        {
            mH->writeLine(statement);
        }

        data_avail = true;
    }

    mH->writeLine();
    mH->writeLine(macro3 + " // " + test + "_H");

    for(const auto& line : unit->mHeaderFooter)
    {
        mH->writeLine("// " + line);
    }

    return data_avail;
}

bool Writer::writeImpl(const Unit* unit)
{
    bool data_avail = false;

    if(mBanner)
        mC->writeLine(unit->Banner("cpp"));

    for(const auto& line : unit->mImplBanner)
    {
        mC->writeLine("// " + line);
    }

    mC->writeLine();

    mC->writeInclude(unit->mName + ".h");

    if(!unit->mImplIncludes.isEmpty())
    {
        mC->writeLine();

        for(const auto& pair : sorted(unit->mImplIncludes))
        {
            mC->writeInclude(pair.second, pair.first);
        }
        data_avail = true;
    }

    if(!unit->mImplBeginBlock.isEmpty())
    {
        mC->writeLine();

        for(const auto& statement : unit->mImplBeginBlock)
            mC->writeLine(statement);

        data_avail = true;
    }

    if(!unit->mVariables.isEmpty())
    {
        mC->writeLine();

        for(const auto& var : sorted(unit->mVariables))
        {
            if(var.mHeaderDecl)
                continue;

            if(var.mIsConstexpr)
                continue;

            if(var.mIsStatic && var.mIsConst)
                continue;

            QString init_str;

            if(!var.mInitMembers.empty())
            {
                init_str.append("(");

                for(const auto& init : var.mInitMembers)
                {
                    init_str.append(init);
                    init_str.append(",");
                }

                init_str.back() = ')';
            }

            init_str.append(";");

            data_avail = true;

            mC->writeLine(var.mType->mName + referenceQualifier(var.mIsReference)+" " + var.mName + init_str);
        }
    }

    if(!unit->mTypes.isEmpty())
    {
        mC->writeLine();

        for(const auto& clas : unit->mTypes)
        {
            if(!mStub)
            {
                if(clas.mGenerateImpl)
                {
                    writeClassImpl(clas);
                }
            }
            else
            {
                for(const auto& func : clas.mMemberFunctions)
                {
                    QStringList function_arguments;

                    for (auto i : func.mParameters)
                        function_arguments.append(i.mType->mName + " " + i.mName);

                    QString mReturnType = func.mReturnType == nullptr ? "void" : func.mReturnType->mName;

                    mC->writeLine(mReturnType + " " + clas.mName + "::" + func.mFunctionName + "(" + function_arguments.join(", ") + ")" + (func.mConst ? " const" : "") + (func.mNoExcept ? " noexcept" : ""));

                    mC->beginBlock();

                    if(func.mReturnStatement.first)
                        mC->writeLine("return " + func.mReturnStatement.second + ";");

                    mC->endBlock(false);
                    mC->writeLine();
                }
                if(clas.mGenerate)
                {
                    mC->writeLine(writeClassDeclaration(clas));
                    mC->beginBlock();
                    mC->endBlock(true);
                }
            }
        }
        data_avail = true;
    }

    if(!unit->mArrays.isEmpty())
    {
        for(const auto& arr : unit->mArrays)
        {
            if(!mStub)
            {
                if(!arr.mGenerateHeader)
                {
                    writeArrayImpl(arr, mC);
                    data_avail = true;
                }
            }
            else
            {
                 if(!arr.mGenerateHeader)
                 {
                     mC->writeLine();
                     mC->writeLine(QString("%1%2std::array<%3, %4> %5")
                                   .arg(staticQualifier(arr.mIsStatic),
                                        (arr.mIsConst ? constQualifier(arr.mIsConst) : constexprQualifier(arr.mIsConstExpr)),
                                        arr.mType.first,
                                        arr.mType.second,
                                        arr.mName));
                     mC->beginBlock();
                     mC->endBlock(true);
                 }
            }
        }
    }

    if(!unit->mMap.isEmpty())
    {
        for(const auto& map : unit->mMap)
        {
            if(!mStub)
            {
                if(!map.mGenerateHeader)
                {
                    writeMapImpl(map, mC);
                }
            }
            else
            {
                if(!map.mGenerateHeader)
                {
                    mC->writeLine();

                    if(map.mSuppressComment.first)
                        mH->writeLine(QString("// parasoft-begin-suppress  %1 GM FALSE POSITIVE Suppressing manually").arg(map.mSuppressComment.second));

                    mC->writeLine(QString("%1%2std::%6<%3, %4> %5")
                                  .arg(staticQualifier(map.mIsStatic),
                                       constQualifier(map.mIsConst),
                                       map.mType.first,
                                       map.mType.second,
                                       map.mName,(map.mMultiMap ? "multimap" : "map")));
                    mC->beginBlock();
                    mC->endBlock(true);

                    if(map.mSuppressComment.first)
                        mH->writeLine(QString("// parasoft-end-suppress  %1 GM FALSE POSITIVE Suppressing manually").arg(map.mSuppressComment.second));
                }
            }
            data_avail = true;
        }
    }

    if(!unit->mFunctions.isEmpty())
    {
        for(const auto& func : unit->mFunctions)
        {
            mC->writeLine();

            if(!mStub)
            {
                if(!func.mGenerateHeader)
                {
                    writeFunctionImpl(func);
                }
            }
            else
            {
                if(!func.mGenerateHeader)
                {
                    if(func.mDllexport == true)
                        mC->writeLine("DECL_DLL_SPEC");

                    mC->writeLine(getFunctionSignature(func));
                    mC->beginBlock();

                    if(func.mReturnStatement.first)
                        mC->writeLine("return " + func.mReturnStatement.second + ";");

                    mC->endBlock(false);
                }
            }
            data_avail = true;
        }
    }

    if(!unit->mImplEndBlock.isEmpty())
    {
        mC->writeLine();

        for(const auto& statement : unit->mImplEndBlock)
            mC->writeLine(statement);

        data_avail = true;
    }

    for(const auto& line : unit->mImplFooter)
    {
        mC->writeLine("// " + line);
    }

    return data_avail;
}

QString Writer::toQString(Access access)
{
    if(access == cw::Access::Protected)
        return QString("protected");

    if(access == cw::Access::Public)
        return  QString("public");

    return QString("private");

}

QString Writer::removeFileBanner(const QString& input_content, const QStringList& banner_lines)
{
    QStringList lines = input_content.split('\n'); // Split input content into lines
    QStringList filtered_lines;
    int count = 0;

    for (const QString& line : lines)
    {
        bool is_banner = false;

        // Only check for banner lines within the first 5 lines
        if (count <= 5)
        {
            // Use QString::compare for exact match (case-sensitive comparison)
            for (const QString& banner_line : banner_lines)
            {
                if (QString::compare(line.trimmed(), banner_line.trimmed(), Qt::CaseSensitive) == 0)
                {
                    is_banner = true;
                    break;
                }
            }
            count += 1;
        }

        // Append the line to filtered_lines only after the first 5 lines
        if (count > 5 && !is_banner)
        {
            filtered_lines.append(line);
        }
    }


    return filtered_lines.join('\n'); // Return the filtered content
}

QString Writer::removeOnlyFileBanner(const Unit* unit, const QString& fileText)
{
    QStringList fileLines = fileText.split('\n'); // Split the content into lines
    QStringList bannerLines = QStringList::fromVector(unit->mHeaderBanner); // Get the banner lines
    QStringList processedLines;

    int count = 0;

    for (const QString& line : fileLines)
    {
        bool is_banner = false;

        // Only check for banner lines within the first 5 lines
        if (count <= 5)
        {
            // Use QString::compare for exact match (case-sensitive comparison)
            for (const QString& banner_line : bannerLines)
            {
                if (QString::compare(line.trimmed(), banner_line.trimmed(), Qt::CaseSensitive) == 0)
                {
                    is_banner = true;
                    break;
                }
            }
            count += 1;
        }

        // Append the line to filtered_lines only if it is not a banner
        if (count>5 && !is_banner)
        {
            processedLines.append(line);
        }
    }
    return processedLines.join('\n'); // Join processed lines into a single string
}

void Writer::generate(Unit* unit)
{
    const auto h_file = unit->mPath + "/" + unit->mName + ".h";
    const auto cpp_file = unit->mPath + "/" + unit->mName + ".cpp";

    QString h_file_text;
    QString cpp_file_text;
    QString headerBannerremoval;
    QString sourceBannerremoval;

    QFile header_file(h_file);
    QFile source_file(cpp_file);

    QString header_data;
    QString source_data;

    if(header_file.open(QIODevice::Text | QIODevice::ReadOnly))
    {
        QString file_content = header_file.readAll();
        QStringList headerBannerList = QStringList::fromVector(unit->mHeaderBanner);
        header_data = removeFileBanner(file_content, headerBannerList);

        header_file.close();
    }

    if(source_file.open(QIODevice::Text | QIODevice::ReadOnly))
    {
        QString file_content = source_file.readAll();
        QStringList headerBannerList = QStringList::fromVector(unit->mHeaderBanner);
        source_data = removeFileBanner(file_content, headerBannerList);

        source_file.close();
    }

    QStringList headerBannerList = QStringList::fromVector(unit->mHeaderBanner);

    mH = new WriterHelper(h_file_text);
    mC = new WriterHelper(cpp_file_text);
    unit->mHeaderHasData = writeHeader(unit);
    unit->mImplHasData = writeImpl(unit);

    delete mH;
    delete mC;

    if(unit->mHeaderHasData)
    {
        headerBannerremoval = removeOnlyFileBanner(unit, h_file_text);
        auto header_data_compare = filesCompare(header_data, headerBannerremoval);
        if (!header_data_compare)
        {
            WriteFile(h_file, h_file_text);
        }
    }
    else
    {
        QFile::remove(h_file);
    }
    if(unit->mImplHasData)
    {
        sourceBannerremoval = removeOnlyFileBanner(unit, cpp_file_text);
        auto source_data_compare = filesCompare(source_data, sourceBannerremoval);
        if (!source_data_compare)
        {
            WriteFile(cpp_file, cpp_file_text);
        }
    }
    else
    {
        QFile::remove(cpp_file);
    }

}

void Writer::setBannerAttribute(bool banner)
{
    mBanner = banner;
}

void Writer::setStubAttribute(bool stub)
{
    mStub = stub;
}

void Writer::addEnum(const QString& name,
                     const QVector<QPair<QString, QString>>& values,
                     bool strongly_typed,
                     QString enum_type)
{
    auto type_dec = [&](){
        if(!enum_type.isEmpty())
            return QString(": %1").arg(enum_type);

        return QString();
    };
    mH->writeLine(QString("enum %1%2 %3").arg(strongly_typed ? "class " : "", name).arg(type_dec()));

    mH->beginBlock();
    {
        if(!values.isEmpty())
        {
            for(const auto& value : values)
            {
                const auto assignment = value.first.isEmpty()
                                        ? QString()
                                        : QString(" = %1").arg(value.first);

                mH->writeLine(QString("%1%2,").arg(value.second, assignment));
            }
        }
    }
    mH->endBlock(true);
}

void Writer::addStructures(const Structures& struc)
{
    mH->writeLine(QString("struct %1 %2").arg(struc.mName , (struc.mFinal ? " final" : "")));

    mH->beginBlock();
    {
        for(const auto& member : struc.mMembers)
        {
            if(!member.mGenerateConstructor)
            {
                mH->writeLine(QString("%1 %2 %3%4;")
                              .arg(constQualifier(member.mIsConst),
                                   member.mType->mName,
                                   member.mName,
                                   initVal(member.mInitVal)));
            }
            else
            {
                mH->writeLine("\n");
                mH->writeLine(QString(" %1(%2 %3) : %4{}")
                              .arg(member.mName,
                                   constQualifier(member.mIsConst),
                                   member.mType->mName,
                                   member.mInitVal));
            }
        }
    }
    mH->endBlock(true);
}

QString Writer::getFunctionSignature(const Function& func)
{
    QStringList function_arguments;

    for (auto i : func.mParameters)
    {
        function_arguments.append(QString("%1%2%3 %4")
                                  .arg(constQualifier(i.mIsConst),
                                       i.mType->mName,
                                       referenceQualifier(i.mIsReference),
                                       i.mName));
    }

    QString mReturnType = func.mReturnType == nullptr ? "void" : func.mReturnType->mName;

    if(func.mOverride == true)
        return virtualQualifier(func.mVirtual) +
               mReturnType + " " + (func.mIsReference ? "&":"") +
               func.mFunctionName + "(" +
               function_arguments.join(", ") + ")" +
               (func.mConst ? " const" : "") +
               (func.mNoExcept ? " noexcept" : "") + " override";
    else
        return virtualQualifier(func.mVirtual) +
               mReturnType + " " + (func.mIsReference ? "&":"") +
               func.mFunctionName + "(" +
               function_arguments.join(", ") + ")" +
               (func.mConst ? " const" : "") +
               (func.mNoExcept ? " noexcept" : "");
}

void Writer::writeFunctionImpl(const Function& func)
{
    if(func.mDllexport == true)
        mC->writeLine("DECL_DLL_SPEC");

    mC->writeLine(getFunctionSignature(func));

    mC->beginBlock();

    for(const auto& statement : func.mStatements)
    {
            mC->writeLine(statement);
    }

    for(const auto& statement : func.mStatement)
    {
        for(const auto& statement1 : statement)
        {
            mC->writeLine(statement1);
        }
    }

    if(func.mReturnStatement.first)
        mC->writeLine("return " + func.mReturnStatement.second + ";");

    mC->endBlock(false);
}

void Writer::writeLangFunctionImpl(const Function& func)
{
    if(func.mDllexport == true)
        mH->writeLine("__declspec(dllexport)");

    mH->writeLine(getFunctionSignature(func));

    mH->beginBlock();

    for(const auto& statement : func.mStatements)
    {
            mH->writeLine(statement);
    }

    for(const auto& statement : func.mStatement)
    {
        for(const auto& statement1 : statement)
        {
            mH->writeLine(statement1);
        }
    }

    if(func.mReturnStatement.first)
        mH->writeLine("return " + func.mReturnStatement.second + ";");

    mH->endBlock(false);
}
void Writer::writeFunctionHeader(const Function& func)
{

    if(func.mDllexport == true)
        mH->writeLine("#ifdef __cplusplus\n"
                      "extern \"C\" { \n"
                      "#endif\n\n"
                      "DECL_DLL_SPEC " + getFunctionSignature(func) + ";\n\n" +
                      "#ifdef __cplusplus\n"
                      "}\n"
                      "#endif\n\n");
    else
        mH->writeLine(getFunctionSignature(func) + ";");
}

QString Writer::writeClassDeclaration(const Type& clas)
{
    if(clas.mInheritance.empty())
        return QString("class") + " " + clas.mName + (clas.mFinal ? " final" : "");

    QStringList inheritances;
    for(const auto& i : clas.mInheritance)
        inheritances.push_back(toQString(i.first) + " " + i.second);

    return QString("class") + " " + clas.mName + (clas.mFinal ? " final" : "") + " : " + inheritances.join(", ");
}

void Writer::writeClassImpl(const Type& clas)
{
    mC->writeLine();

    QStringList init_lists;

    if(!clas.mGenerate && !clas.mGenerateConstructor)
    {
        if(!clas.mBaseclassInit.second.isEmpty())
            init_lists.append(QString("%1(%2)").arg(clas.mBaseclassInit.first,clas.mBaseclassInit.second.join(',')));
    }

    for(const auto& var : sorted(clas.mMembers))
    {
        if(!var.mInitVal.isEmpty())
            init_lists.append(QString("%1%2%3%4(%5)").arg(constQualifier(var.mIsConst),
                                                          staticQualifier(var.mIsStatic),
                                                          var.mName,
                                                          referenceQualifier(var.mIsReference),
                                                          var.mInitVal));
    }

    if(!clas.mGenerate && !clas.mGenerateConstructor)
    {
        if(init_lists.isEmpty())
            mC->writeLine(clas.mName+"::"+clas.mName+"()" + (clas.mNoExcept ? " noexcept" : ""));
        else
            mC->writeLine(clas.mName+"::"+clas.mName+"():"+ QString(init_lists.join(',')) + (clas.mNoExcept ? "  noexcept" : ""));
    }

    if(!clas.mGenerate && !clas.mGenerateConstructor)
    {
        mC->beginBlock();
        mC->endBlock(false);
        mC->writeLine();
    }

    for(const auto& func : clas.mMemberFunctions)
    {
        QStringList function_arguments;

        for (auto i : func.mParameters)
            function_arguments.append(constQualifier(i.mIsConst)+
                                      staticQualifier(i.mIsStatic)+
                                      i.mType->mName +
                                      referenceQualifier(i.mIsReference) +
                                      " " +
                                      i.mName);

        QString mReturnType = func.mReturnType == nullptr ? "void" : func.mReturnType->mName;

        mC->writeLine(mReturnType + " " + clas.mName + "::" + func.mFunctionName + "(" + function_arguments.join(", ") + ")" + (func.mConst ? " const" : "") + (func.mNoExcept ? " noexcept" : ""));


        mC->beginBlock();

        for(const auto& statement : func.mStatements)
            mC->writeLine(statement);

        if(func.mReturnStatement.first)
            mC->writeLine("return " + func.mReturnStatement.second + ";");


        mC->endBlock(false);
        mC->writeLine();
    }
}

void Writer::writeClassHeader(const Type& cals)
{
    mH->writeLine();
    mH->writeLine(writeClassDeclaration(cals));
    mH->beginBlock();

    mH->writeLine("public:");
    if(cals.mGenerate)
    {
        mH->writeLine(QString("using %1::%2;").arg(cals.mBaseclassInit.first, cals.mBaseclassInit.first));
    }
    if(!cals.mGenerate && !cals.mGenerateConstructor)
    {
        mH->writeLine(QString("%1() %2;").arg(cals.mName, cals.mNoExcept ? "noexcept" : ""));
    }

    QStringList init_members;

    for(const auto& variable : sorted(cals.mMembers))
    {
        auto init_params = variable.mInitMembers.join(",");

        if(!init_params.isEmpty())
        {
            if (cals.mInheritance.isEmpty() && init_members.isEmpty())
            {
                init_members.append(": " + variable.mName + "(" + init_params + ")");
            }
            else
            {
                init_members.append(", " + variable.mName + "(" + init_params + ")");
            }
        }
    }
    for(const auto& variable : sorted(cals.mMembers))
    {
        if(variable.mGenerateConstructor)
        {
            mH->writeLine(QString(cals.mName + "(" + variable.mType->mName + " " + variable.mInitVal + ") %1").arg(cals.mNoExcept ? "noexcept " : " "));

            if (!cals.mInheritance.isEmpty())
            {
                mH->writeLine(": " + cals.mBaseclassInit.first + "(" + variable.mInitVal + ")");
            }

            if(!variable.mVariableInitialization)
            {
                for(const auto& init : init_members)
                {
                    mH->writeLine(init);
                }
            }

            mH->beginBlock();
            mH->endBlock();
            mH->writeLine();
        }
    }

    handleVariableAccess(cals.mMembers);

    for(const auto& fun : cals.mMemberFunctions)
    {
        mH->writeLine(toQString(fun.mAccess) + ":");
        writeFunctionHeader(fun);
    }

    mH->endBlock(true);
}

void Writer::writeArrayImpl(const Arrays& arrays, WriterHelper* writer)
{
    writer->writeLine();

    if(arrays.mFunctionFormat)
    {
        QString const_expr;

        if((!arrays.mIsConst && arrays.mIsConstExpr) || arrays.mIsConst)
            const_expr = "const ";
        else if((arrays.mIsConst && !arrays.mIsConstExpr) || arrays.mIsConstExpr)
            const_expr = "constexpr ";

        writer->writeLine(QString("%1%2std::array<%3, %4> &%5_data()")
                          .arg(staticQualifier(arrays.mFunctionIsStatic),
                               const_expr,
                               arrays.mType.first,
                               arrays.mType.second,
                               arrays.mName));
        writer->beginBlock();

    }
    writer->writeLine(QString("%1%2std::array<%3, %4> %5")
                      .arg(staticQualifier(arrays.mIsStatic),
                           (arrays.mIsConst ? constQualifier(arrays.mIsConst) : constexprQualifier(arrays.mIsConstExpr)),
                           arrays.mType.first,
                           arrays.mType.second,
                           arrays.mName));

    writer->beginBlock();

    writer->beginBlock();

    for(const auto& val : arrays.mValue)
    {
        QStringList values;

        for( const auto& single : val)
        {
            values.append(single);
        }
        auto val_count = values.count();

        if(arrays.mIsSubArray == true)
        {
            writer->writeLine(QString("{%1},").arg(values.join("")));
        }
        else
        {
            if(val_count == 1)
                writer->writeLine(QString("%1").arg(values.join("")));
            else
                writer->writeLine(QString("%1").arg(values.join(",\n    ")));
        }
    }

    writer->endBlock(false);

    if(arrays.mFunctionFormat)
    {
        writer->endBlock(true);
        writer->writeLine(QString("return %1;").arg(arrays.mName));
        writer->endBlock(false);
    }
    else
    {
        writer->endBlock(true);
    }

}

void Writer::writeMapImpl(const Map &maps, WriterHelper* writer)
{
    writer->writeLine();

    QString map_format,func_name;

    if(maps.mSuppressComment.first)
        writer->writeLine(QString("// parasoft-begin-suppress  %1 GM FALSE POSITIVE Suppressing manually").arg(maps.mSuppressComment.second));

    if(maps.mUsingFormat.first)
        map_format = maps.mUsingFormat.second;
    else
        map_format = QString("std::%1map<%2 , %3>").arg((maps.mMultiMap ? "multi" : ""), maps.mType.first, maps.mType.second);

    if(maps.mFunctionFormat)
    {
        func_name = "&" + maps.mName + "_data()";

        writer->writeLine(QString("%1%2%5%3 %4")
                          .arg(staticQualifier(maps.mFunctionStatic),
                               constQualifier(maps.mIsConst),
                               map_format,
                               func_name,
                               constQualifier(maps.mFunctionConst)));
        writer->beginBlock();
    }

    writer->writeLine(QString("%1%2%3 %4")
                      .arg(staticQualifier(maps.mIsStatic),
                           constQualifier(maps.mIsConst),
                           map_format,
                           maps.mName));

    writer->beginBlock();
    for(const auto& val : maps.mValue)
    {
        writer->writeLine(QString("{%1,%2},").arg(val.first, val.second));
    }

    for(const auto& val : maps.mArrayValue)
    {
        auto map_val = QString("{%1, { ").arg(val.first);

        for(auto& plural_val : val.second)
        {
           map_val.append(QString("%1,").arg(plural_val));
        }
        map_val.append("} },");

        writer->writeLine(map_val);
    }

    if(maps.mFunctionFormat)
    {
        writer->endBlock(true);
        writer->writeLine(QString("return %1;").arg(maps.mName));
        writer->endBlock(false);
    }
    else
    {
        writer->endBlock(true);
    }

    if(maps.mSuppressComment.first)
        writer->writeLine(QString("// parasoft-end-suppress  %1 GM FALSE POSITIVE Suppressing manually").arg(maps.mSuppressComment.second));
}

void Writer::writeVectorImpl(const Vector &vector, WriterHelper *writer)
{
    writer->writeLine();
    writer->writeLine(QString("%1%2std::vector<%3> %4 = ")
                      .arg(staticQualifier(vector.mIsStatic),
                           constQualifier(vector.mIsConst),
                           vector.mType,
                           vector.mName));

    writer->beginBlock();
    for(const auto& val : vector.mValue)
    {
        writer->writeLine(QString("{%1},").arg(val));
    }
    writer->endBlock(true);
}

bool Writer::filesCompare(const QString &file_data_before, const QString &file_data_after)
{
    bool data_compare = true;

    if(QString::compare(file_data_before,file_data_after,Qt::CaseSensitive) != 0)
        data_compare = false;

    return data_compare;
}

void Writer::handleVariableAccess(const QVector<Variable>& variables )
{
    mPrivateMemberVariables.clear();
    mPublicMembervariables.clear();
    mProtectedMemberVariables.clear();

    for(const auto& variable : sorted(variables))
    {
        QString var;

        QString init_mem = variable.mInitMembers.join(",");

        if(!variable.mGenerateConstructor)
        {
            var = QString("\t%2%3%4%5 %6;").arg(
                    constexprQualifier(variable.mIsConstexpr),
                    staticQualifier(variable.mIsStatic),
                    constQualifier(variable.mIsConst),
                    variable.mType != nullptr ? variable.mType->mName  : QString(),
                    variable.mVariableInitialization ? variable.mName + "{" + init_mem + "}" : variable.mName);

            if(variable.mAccess == Access::Public)
                mPublicMembervariables.append(var);
            else if(variable.mAccess == Access::Protected)
                mProtectedMemberVariables.append(var);
            else
                mPrivateMemberVariables.append(var);
        }

    }

    if(!mPublicMembervariables.isEmpty())
        mH->writeLine("public:");

    for(const auto& public_var : mPublicMembervariables)
    {
        mH->writeLine(public_var);
    }

    if(!mProtectedMemberVariables.isEmpty())
        mH->writeLine("protected:");

    for(const auto& protected_var : mProtectedMemberVariables)
    {
        mH->writeLine(protected_var);
    }

    if(!mPrivateMemberVariables.isEmpty())
        mH->writeLine("private:");

    for(const auto& private_var : mPrivateMemberVariables)
    {
        mH->writeLine(private_var);
    }
}

}  // namespace cw
